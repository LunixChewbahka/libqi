#pragma once
/*
**  Copyright (C) 2012 Aldebaran Robotics
**  See COPYING for the license
*/

#ifndef _QIMESSAGING_GENERICVALUE_HPP_
#define _QIMESSAGING_GENERICVALUE_HPP_

#include <qimessaging/type.hpp>
namespace qi {

class GenericList;
class GenericMap;
class GenericObject;
class GenericList;


/** Class that holds any value, with informations to manipulate it.
 *  Operator = makes a shallow copy.
 *
 */
class QIMESSAGING_API GenericValue
{
public:
  GenericValue();
  GenericValue(Type* type, void* value) : type(type), value(value) {}
  /** Return the typed pointer behind a GenericValue. T *must* be the type
   * of the value.
   * @return a pointer to the value as a T or 0 if value is not a T.
   * @param check if false, does not validate type before converting
   */
  template<typename T> T* ptr(bool check = true);
  /// @return the pair (convertedValue, trueIfCopiedAndNeedsDestroy)
  std::pair<GenericValue, bool> convert(Type* targetType) const;
  /// Helper function that converts and always clone
  GenericValue convertCopy(Type* targetType) const;
  GenericValue clone() const;
  void serialize(ODataStream& os) const;
  std::string signature() const;
  void destroy();
  Type::Kind kind() const;

  int64_t asInt() const;
  float   asFloat() const;
  double  asDouble() const;

  std::string asString() const;

  GenericList asList() const;
  GenericMap  asMap() const;
  GenericObject asObject() const;

  template<typename T> T as() const;
  // Helper function to get T from a value
  template<typename T> T as(const T&) const;

  Type*   type;
  void*   value;
};

/// Convert any value to the correct associated Value
template<typename T> GenericValue toValue(const T& v);


/** Generates GenericValue from everything transparently.
 * To be used as type of meta-function call argument
 *
 *  Example:
 *    void metaCall(ValueGen arg1, ValueGen arg2);
 *  can be called with any argument type:
 *    metaCall("foo", 12);
 */
class AutoGenericValue: public GenericValue
{
public:
  AutoGenericValue ();
  AutoGenericValue(const AutoGenericValue & b);

  template<typename T> AutoGenericValue(const T& ptr);
};


}

#include <qimessaging/details/genericvalue.hxx>

#endif  // _QIMESSAGING_GENERICVALUE_HPP_
