// Copyright 2016 Alex Silva Torres
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "decl-class-object.h"

#include "object-factory.h"

namespace shpp {
namespace internal {

AbstractMethod::AbstractMethod(const AbstractMethod& method)
  : num_params_(method.num_params_)
  , num_default_params_(method.num_default_params_)
  , variadic_(method.variadic_) {}

AbstractMethod& AbstractMethod::operator=(const AbstractMethod& method) {
  num_params_ = method.num_params_;
  num_default_params_ = method.num_default_params_;
  variadic_ = method.variadic_;

  return *this;
}

AbstractMethod::AbstractMethod(AbstractMethod&& method)
  : num_params_(method.num_params_)
  , num_default_params_(method.num_default_params_)
  , variadic_(method.variadic_) {}

AbstractMethod& AbstractMethod::operator=(AbstractMethod&& method) {
  num_params_ = method.num_params_;
  num_default_params_ = method.num_default_params_;
  variadic_ = method.variadic_;

  return *this;
}

bool AbstractMethod::operator==(const FuncObject& func) const {
  if (variadic_) {
    return (func.NumParams() == num_params_) &&
         (func.NumDefaultParams() == num_default_params_) &&
         func.CVariadic() == variadic_;
  }

  // the number of params include the number of default_params
  return (func.NumParams() == num_params_) &&
         (func.CVariadic() == variadic_);
}

bool AbstractMethod::operator!=(const FuncObject& func) const {
  return !this->operator==(func);
}

bool AbstractMethod::operator==(const AbstractMethod& func) const {
  if (variadic_) {
    return (func.num_params_ == num_params_) &&
         (func.num_default_params_ == num_default_params_) &&
         func.variadic_ == variadic_;
  }

  // the number of params include the number of default_params
  return (func.num_params_ == num_params_) &&
         (func.variadic_ == variadic_);
}

bool AbstractMethod::operator!=(const AbstractMethod& func) const {
  return !this->operator==(func);
}

DeclClassType::DeclClassType(const std::string& name, ObjectPtr obj_type,
    SymbolTableStack&& sym_table, ObjectPtr base,
    InterfacesList&& ifaces, bool abstract)
    : TypeObject(name, obj_type, std::move(sym_table), base, std::move(ifaces))
    , abstract_(abstract) {
  symbol_table_stack().Push(SymbolTablePtr(new SymbolTable(
      SymbolTable::TableType::CLASS_TABLE)));

  // if there is no base type, so we don't need verify abstract methods
  // from base class
  if (!base) {
    return;
  }

  // the class TypeObject already check if the base is a type, so we can
  // cast this object to TypeObject without any problem with segfault
  TypeObject& type_base = static_cast<TypeObject&>(*base);

  // only user declared class has abstract methods
  if (!type_base.Declared()) {
    return;
  }

  // here we are surely that the object is a declared class
  DeclClassType& decl_base = static_cast<DeclClassType&>(*base);

  // insert the abstract methods from the base class
  for (auto& method: decl_base.AbstractMethods()) {
    auto it = abstract_methods_.find(method.first);

    // abstract method must be have unique name
    if (it != abstract_methods_.end()) {
      throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
          boost::format("not allowed same name '%1%' method on class")
          %method.first);
    }

    abstract_methods_.insert(std::pair<std::string, AbstractMethod>(method.first,
        method.second));
  }
}

void DeclClassType::AddAbstractMethod(const std::string& name,
    AbstractMethod&& method) {
  // abstract class can not be instantiated
  if (!abstract_) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
        boost::format("not allowed abstract '%1%' method on no abstract class")
        %name);
  }

  // not allowed insert methods with same names
  auto it = abstract_methods_.find(name);
  if (it != abstract_methods_.end()) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
        boost::format("not allowed same name '%1%' method on class")
        %name);
  }

  // verify is exists some impelemented method with same name
  if (ExistsAttr(name)) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
        boost::format("not allowed same name '%1%' attribute on class")
        %name);
  }

  abstract_methods_.insert(std::pair<std::string, AbstractMethod>(name,
      std::move(method)));
}

void DeclClassType::CheckAbstractMethodsCompatibility() {
  // if it is an abstract class, the class doesn't need
  // implement abstract methods from this or from
  // the base class
  if (abstract_) {
    return;
  }

  // verify if all abstract methods are implemented
  for (auto& method: AbstractMethods()) {
    ObjectPtr fobj = SearchAttr(method.first);

    // check if the attribute is really a method
    if (fobj->type() != ObjectType::FUNC) {
      throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
          boost::format("attribute '%1%' is not a method")%method.first);
    }

    // check if the abstract method is equal the implemented
    if (method.second != static_cast<FuncObject&>(*fobj)) {
      throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
          boost::format("method '%1%' has wrong number of parameters")
          %method.first);
    }
  }
}

void DeclClassType::CheckInterfaceCompatibility() {
  // verify if all methods from interfaces are implemented
  for (auto& iface: Interfaces()) {
    if (iface->type() != ObjectType::DECL_IFACE) {
      throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
          boost::format("only interface supported"));
    }

    for (auto& method: static_cast<DeclInterface&>(*iface).Methods()) {
      // if the method in the interface was declared as abstract
      // we don't need to check if the method was implemented
      // in this class, because some derived class has to
      // implement it
      auto it = abstract_methods_.find(method.first);
      if (it != abstract_methods_.end()) {
        if (it->second == method.second) {
          continue;
        }
      }

      // search if the abstract method from interface was implemented
      // on this class or in any class which this class is derived
      ObjectPtr fobj = SearchAttr(method.first);

      if (fobj->type() != ObjectType::FUNC) {
        throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
            boost::format("attribute '%1%' is not a method")%method.first);
      }

      // check if the method in interface is equal the implemented
      if (method.second != static_cast<FuncObject&>(*fobj)) {
        throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
            boost::format("method '%1%' has wrong number of parameters")%method.first);
      }
    }
  }
}

// constructor for declared class call __init__ method from
// symbol table, and create an DeclClassObject, this object
// has a symbol table stack to store attributes
ObjectPtr DeclClassType::Constructor(Executor* parent, Args&& params,
    KWArgs&& kw_params) {
  if (abstract_) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
        boost::format("abstract class '%1%' can not be instantiated")
        %ObjectName());
  }

  ObjectFactory obj_factory(symbol_table_stack());
  ObjectPtr obj_self(obj_factory.NewDeclObject(this->name()));

  if (symbol_table_stack().Exists("__init__")) {
    ObjectPtr obj_init = symbol_table_stack().Lookup("__init__", false)
      .SharedAccess();

    if (obj_init->type() == ObjectType::FUNC) {
      params.insert(params.begin(), obj_self);
      static_cast<FuncObject&>(*obj_init).Call(parent, std::move(params),
          std::move(kw_params));
    }
  }

  return obj_self;
}

ObjectPtr DeclClassType::CallObject(const std::string& name,
                                 ObjectPtr self_param) {
  // search on this class and all base classes
  ObjectPtr obj = SearchAttr(name);

  if (obj->type() == ObjectType::FUNC) {
    ObjectFactory obj_factory(symbol_table_stack());

    // the function wrapper insert the object self_param as the first param
    // it works like self argument
    return ObjectPtr(obj_factory.NewWrapperFunc(obj, self_param));
  }

  return obj;
}

std::shared_ptr<Object>& DeclClassType::AttrAssign(
    std::shared_ptr<Object> /*self*/, const std::string& name) {
  ObjectPtr& att_obj = symbol_table_stack().Lookup(name, true).Ref();

  return att_obj;
}

std::shared_ptr<Object> DeclClassType::Attr(std::shared_ptr<Object> self,
                              const std::string& name) {
  ObjectPtr att_obj = SearchAttr(name);
  return att_obj;
}

std::shared_ptr<Object> DeclClassObject::Attr(std::shared_ptr<Object> self,
                              const std::string& name) {
  // first check it the attribute exists on object symbol table
  if (symbol_table_stack().Exists(name)) {
    ObjectPtr att_obj = symbol_table_stack().Lookup(name, false).SharedAccess();

    // functions on object are handle to insert this parameter
    if (att_obj->type() == ObjectType::FUNC) {
      return static_cast<DeclClassType&>(*ObjType()).CallObject(name, self);
    }

    return att_obj;
  }

  // if the attribute is not on object symbol table search it on type class
  // and all base class
  ObjectPtr att_obj = static_cast<TypeObject&>(*ObjType()).SearchAttr(name);

  if (att_obj->type() == ObjectType::FUNC) {
    // if the function is not declared, just return it
    if (!static_cast<FuncObject&>(*att_obj).Declared()) {
      return att_obj;
    }

    // if the function is static don't wrapper the function to pass the
    // parameter this
    if (static_cast<FuncDeclObject&>(*att_obj).Static()) {
      throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
            boost::format("static method '%1%' must not be called by object")%name);
    }

    ObjectFactory obj_factory(symbol_table_stack());

    // the function wrapper insert the object self_param as the first param
    // it works like self argument
    return ObjectPtr(obj_factory.NewWrapperFunc(att_obj, self));
  }

  return att_obj;
}

std::shared_ptr<Object>& DeclClassObject::AttrAssign(
    std::shared_ptr<Object> /*self*/, const std::string& name) {
  ObjectPtr& att_obj = symbol_table_stack().Lookup(name, true).Ref();

  return att_obj;
}

ObjectPtr DeclClassObject::Add(ObjectPtr obj) {
  return Caller("__add__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Sub(ObjectPtr obj) {
  return Caller("__sub__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Mult(ObjectPtr obj) {
  return Caller("__mul__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Div(ObjectPtr obj) {
  return Caller("__div__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::DivMod(ObjectPtr obj) {
  return Caller("__mod__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::RightShift(ObjectPtr obj) {
  return Caller("__rshift__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::LeftShift(ObjectPtr obj) {
  return Caller("__lshift__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Lesser(ObjectPtr obj) {
  return Caller("__lt__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Greater(ObjectPtr obj) {
  return Caller("__gt__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::LessEqual(ObjectPtr obj) {
  return Caller("__le__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::GreatEqual(ObjectPtr obj) {
  return Caller("__ge__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Equal(ObjectPtr obj) {
  return Caller("__eq__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::In(ObjectPtr obj) {
  return Caller("__contains__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::NotEqual(ObjectPtr obj) {
  return Caller("__ne__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::BitAnd(ObjectPtr obj) {
  return Caller("__rand__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::BitOr(ObjectPtr obj) {
  return Caller("__ror__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::BitXor(ObjectPtr obj) {
  return Caller("__rxor__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::BitNot() {
  return Caller("__rinvert__", self_.lock());
}

ObjectPtr DeclClassObject::And(ObjectPtr obj) {
  return Caller("__and__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::Or(ObjectPtr obj) {
  return Caller("__or__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::GetItem(ObjectPtr obj) {
  return Caller("__getitem__", self_.lock(), obj);
}

ObjectPtr DeclClassObject::ObjIter(ObjectPtr /*obj*/) {
  return Caller("__iter__", self_.lock());
}

void DeclClassObject::DelItem(ObjectPtr /*obj*/) {
  Caller("__del__", self_.lock());
}

ObjectPtr DeclClassObject::UnaryAdd() {
  return Caller("__pos__", self_.lock());
}

ObjectPtr DeclClassObject::UnarySub() {
  return Caller("__neg__", self_.lock());
}

ObjectPtr DeclClassObject::Not() {
  return Caller("__invert__", self_.lock());
}

ObjectPtr DeclClassObject::Begin() {
  return Caller("__begin__", self_.lock());
}

ObjectPtr DeclClassObject::End() {
  return Caller("__end__", self_.lock());
}

ObjectPtr DeclClassObject::Next() {
  return Caller("__next__", self_.lock());
}

ObjectPtr DeclClassObject::HasNext() {
  return Caller("__has_next__", self_.lock());
}

ObjectPtr DeclClassObject::Call(Executor*, Args&& params, KWArgs&& kw_params) {
  return Caller("__call__", std::move(params), std::move(kw_params));
}

std::string DeclClassObject::Print() {
  ObjectPtr obj = Caller("__print__", self_.lock());

  if (obj->type() != ObjectType::STRING) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
                      boost::format("print func must return string"));
  }

  return static_cast<StringObject&>(*obj).value();
}

long int DeclClassObject::Len() {
  ObjectPtr obj = Caller("__len__", self_.lock());

  if (obj->type() != ObjectType::INT) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
                      boost::format("__len__ func must return integer"));
  }

  return static_cast<IntObject&>(*obj).value();
}

std::size_t DeclClassObject::Hash() {
  ObjectPtr obj = Caller("__hash__", self_.lock());

  if (obj->type() != ObjectType::INT) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
                      boost::format("__hash__ func must return integer"));
  }

  return static_cast<std::size_t>(static_cast<IntObject&>(*obj).value());
}

ObjectPtr DeclClassObject::ObjBool() {
  return Caller("__bool__", self_.lock());
}

ObjectPtr DeclClassObject::ObjCmd() {
  return Caller("__cmd__", self_.lock());
}

ObjectPtr DeclClassObject::ObjString() {
  return Caller("__str__", self_.lock());
}

ObjectPtr DeclClassObject::Caller(const std::string& fname, Args&& params,
    KWArgs&& kw_params) {
  SymbolTableStack& st =
      static_cast<DeclClassType&>(*ObjType()).SymTableStack();
  ObjectPtr func_obj = st.Lookup(fname, false).SharedAccess();

  if (func_obj->type() != ObjectType::FUNC) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
                      boost::format("symbol %1% must be func")%fname);
  }

  params.insert(params.begin(), self_.lock());

  return static_cast<FuncObject&>(*func_obj).Call(nullptr, std::move(params),
      std::move(kw_params));
}

DeclInterface::DeclInterface(const std::string& name, ObjectPtr obj_type,
    SymbolTableStack&& sym_table,
    std::vector<std::shared_ptr<Object>>&& ifaces)
    : TypeObject(name, obj_type, std::move(sym_table), ObjectPtr(nullptr),
          std::move(ifaces), ObjectType::DECL_IFACE) {
  // insert the methods from the bases interfaces
  // those methods have to be unique
  for (auto& ifc: Interfaces()) {
    for (auto& method: static_cast<DeclInterface&>(*ifc).Methods()) {
      auto it = methods_.find(method.first);
      if (it != methods_.end()) {
        throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
            boost::format("not allowed same name '%1%' method on interface")
            %method.first);
      }

      methods_.insert(std::pair<std::string, AbstractMethod>(method.first,
          method.second));
    }
  }
}

ObjectPtr DeclInterface::Constructor(Executor* parent, Args&&, KWArgs&&) {
  throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
                      boost::format("Interface can not be instantiated"));
}

void DeclInterface::AddMethod(const std::string& name,
    AbstractMethod&& method) {
  // not allowed insert methods with same names
  auto it = methods_.find(name);
  if (it != methods_.end()) {
    throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
        boost::format("not allowed same name '%1%' method on interface")
        %name);
  }

  methods_.insert(std::pair<std::string, AbstractMethod>(name,
      std::move(method)));
}

std::shared_ptr<Object> DeclInterface::Attr(std::shared_ptr<Object> self,
    const std::string& name) {
  throw RunTimeError(RunTimeError::ErrorCode::INCOMPATIBLE_TYPE,
      boost::format("Methods from interface can't be called"));
}

}
}
