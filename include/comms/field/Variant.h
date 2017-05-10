//
// Copyright 2017 (C). Alex Robenko. All rights reserved.
//

// This file is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.


#pragma once

#include "comms/ErrorStatus.h"
#include "comms/field/category.h"
#include "comms/options.h"
#include "basic/Variant.h"
#include "details/AdaptBasicField.h"
#include "comms/details/macro_common.h"
#include "comms/details/variant_access.h"
#include "tag.h"

namespace comms
{

namespace field
{

/// @brief Variants multiple fields into a single field.
/// @details The class wraps nicely multiple fields and provides
///     expected single field API functions, such as length(), read(), write(),
///     valid(). It may be useful when a collection (comms::field::ArrayList) of
///     complex fields is required.
///
///     Refer to @ref sec_field_tutorial_bundle for tutorial and usage examples.
/// @tparam TFieldBase Base class for this field, expected to be a variant of
///     comms::Field.
/// @tparam TMembers All wrapped fields bundled together in
///     <a href="http://en.cppreference.com/w/cpp/utility/tuple">std::tuple</a>.
/// @tparam TOptions Zero or more options that modify/refine default behaviour
///     of the field.@n
///     Supported options are:
///     @li comms::option::DefaultValueInitialiser - All wrapped fields may
///         specify their independent default value initialisers. It is
///         also possible to provide initialiser for the Variant field which
///         will set appropriate values to the fields based on some
///         internal logic.
///     @li comms::option::ContentsValidator - All wrapped fields may specify
///         their independent validators. The bundle field considered to
///         be valid if all the wrapped fields are valid. This option though,
///         provides an ability to add extra validation logic that can
///         observe value of more than one wrapped fields. For example,
///         protocol specifies that if one specific field has value X, than
///         other field is NOT allowed to have value Y.
///     @li comms::option::CustomValueReader - It may be required to implement
///         custom reading functionality instead of default behaviour of
///         invoking read() member function of every member field. It is possible
///         to provide cusom reader functionality using comms::option::CustomValueReader
///         option.
///     @li comms::option::DefaultVariantIndex - By default the Variant field
///         dosn't have any valid contents. This option may be used to specify
///         the index of the default member field.
template <typename TFieldBase, typename TMembers, typename... TOptions>
class Variant
{
    static_assert(comms::util::IsTuple<TMembers>::Value,
        "TMembers is expected to be a tuple of std::tuple<...>");

    static_assert(
        1U < std::tuple_size<TMembers>::value,
        "Number of members is expected to be at least 2.");

    using BasicField = basic::Variant<TFieldBase, TMembers>;
    using ThisField = details::AdaptBasicFieldT<BasicField, TOptions...>;

public:
    /// @brief All the options provided to this class bundled into struct.
    using ParsedOptions = details::OptionsParser<TOptions...>;

    /// @brief Tag indicating type of the field
    using Tag = tag::Variant;

    /// @brief Value type.
    /// @details Same as TMemebers template argument, i.e. it is std::tuple
    ///     of all the wrapped fields.
    using ValueType = typename ThisField::ValueType;

    using Members = typename ThisField::Members;

    /// @brief Default constructor
    /// @details Invokes default constructor of every wrapped field
    Variant() = default;

    /// @brief Constructor
    explicit Variant(const ValueType& val)
      : field_(val)
    {
    }

    /// @brief Constructor
    explicit Variant(ValueType&& val)
      : field_(std::move(val))
    {
    }

    /// @brief Get access to the stored tuple of fields.
    ValueType& value()
    {
        return field_.value();
    }

    /// @brief Get access to the stored tuple of fields.
    const ValueType& value() const
    {
        return field_.value();
    }

    /// @brief Get length required to serialise bundled fields.
    /// @details Summarises all the results returned by the call to length() for
    ///     every field in the bundle.
    /// @return Number of bytes it will take to serialise the field value.
    constexpr std::size_t length() const
    {
        return field_.length();
    }

    /// @brief Get minimal length that is required to serialise all bundled fields.
    /// @return Minimal number of bytes required serialise the field value.
    static constexpr std::size_t minLength()
    {
        return ThisField::minLength();
    }

    /// @brief Get maximal length that is required to serialise all bundled fields.
    /// @return Maximal number of bytes required serialise the field value.
    static constexpr std::size_t maxLength()
    {
        return ThisField::maxLength();
    }

    /// @brief Read field value from input data sequence
    /// @details Invokes read() member function over every bundled field.
    /// @param[in, out] iter Iterator to read the data.
    /// @param[in] size Number of bytes available for reading.
    /// @return Status of read operation.
    /// @post Iterator is advanced.
    template <typename TIter>
    ErrorStatus read(TIter& iter, std::size_t size)
    {
        return field_.read(iter, size);
    }

    /// @brief Write current field value to output data sequence
    /// @details Invokes write() member function over every bundled field.
    /// @param[in, out] iter Iterator to write the data.
    /// @param[in] size Maximal number of bytes that can be written.
    /// @return Status of write operation.
    /// @post Iterator is advanced.
    template <typename TIter>
    ErrorStatus write(TIter& iter, std::size_t size) const
    {
        return field_.write(iter, size);
    }

    /// @brief Check validity of all the bundled fields.
    constexpr bool valid() const {
        return field_.valid();
    }

    std::size_t currentField() const
    {
        return field_.currentField();
    }

    void selectField(std::size_t idx)
    {
        field_.selectField(idx);
    }

    template <typename TFunc>
    void currentFieldExec(TFunc&& func)
    {
        field_.currentFieldExec(std::forward<TFunc>(func));
    }

    template <typename TFunc>
    void currentFieldExec(TFunc&& func) const
    {
        field_.currentFieldExec(std::forward<TFunc>(func));
    }

    template <std::size_t TIdx, typename... TArgs>
    auto initField(TArgs&&... args) -> decltype(std::declval<ThisField>().template initField<TIdx>(std::forward<TArgs>(args)...))
    {
        return field_.template initField<TIdx>(std::forward<TArgs>(args)...);
    }

    template <std::size_t TIdx>
    auto accessField() -> decltype(std::declval<ThisField>().template accessField<TIdx>())
    {
        return field_.template accessField<TIdx>();
    }

    template <std::size_t TIdx>
    auto accessField() const -> decltype(std::declval<const ThisField>().template accessField<TIdx>())
    {
        return field_.template accessField<TIdx>();
    }

    bool currentFieldValid() const
    {
        return field_.currentFieldValid();
    }

    void reset()
    {
        field_.reset();
    }

private:
    ThisField field_;
};

namespace details
{

template <typename TVar>
class VariantEqualityCompHelper
{
public:
    VariantEqualityCompHelper(const TVar& other, bool& result)
      : other_(other),
        result_(result)
    {}

    template <std::size_t TIdx, typename TField>
    void operator()(const TField& field)
    {
        result_ = (field == other_.template accessField<TIdx>());
    }

private:
    const TVar& other_;
    bool& result_;
};

template <typename TVar>
VariantEqualityCompHelper<TVar> makeVariantEqualityCompHelper(TVar& other, bool& result)
{
    return VariantEqualityCompHelper<TVar>(other, result);
}

} // namespace details

/// @brief Equality comparison operator.
/// @param[in] field1 First field.
/// @param[in] field2 Second field.
/// @return true in case fields are equal, false otherwise.
/// @related Variant
template <typename TFieldBase, typename TMembers, typename... TOptions>
bool operator==(
    const Variant<TFieldBase, TMembers, TOptions...>& field1,
    const Variant<TFieldBase, TMembers, TOptions...>& field2)
{
    if (&field1 == &field2) {
        return true;
    }

    if (field1.currentFieldValid() != field2.currentFieldValid()) {
        return false;
    }

    if (!field1.currentFieldValid()) {
        return true;
    }

    if (field1.currentField() != field2.currentField()) {
        return false;
    }

    bool result = false;
    field1.currentFieldExec(details::makeVariantEqualityCompHelper(field2, result));
    return result;
}

/// @brief Non-equality comparison operator.
/// @param[in] field1 First field.
/// @param[in] field2 Second field.
/// @return true in case fields are NOT equal, false otherwise.
/// @related Variant
template <typename TFieldBase, typename TMembers, typename... TOptions>
bool operator!=(
    const Variant<TFieldBase, TMembers, TOptions...>& field1,
    const Variant<TFieldBase, TMembers, TOptions...>& field2)
{
    return field1.value() != field2.value();
}

/// @brief Compile time check function of whether a provided type is any
///     variant of comms::field::Variant.
/// @tparam T Any type.
/// @return true in case provided type is any variant of @ref Variant
/// @related comms::field::Variant
template <typename T>
constexpr bool isVariant()
{
    return std::is_same<typename T::Tag, tag::Variant>::value;
}

/// @brief Upcast type of the field definition to its parent comms::field::Variant type
///     in order to have access to its internal types.
template <typename TFieldBase, typename TMembers, typename... TOptions>
inline
Variant<TFieldBase, TMembers, TOptions...>&
toFieldBase(Variant<TFieldBase, TMembers, TOptions...>& field)
{
    return field;
}

/// @brief Upcast type of the field definition to its parent comms::field::Variant type
///     in order to have access to its internal types.
template <typename TFieldBase, typename TMembers, typename... TOptions>
inline
const Variant<TFieldBase, TMembers, TOptions...>&
toFieldBase(const Variant<TFieldBase, TMembers, TOptions...>& field)
{
    return field;
}

#define COMMS_VARIANT_MEMBERS_ACCESS(...) \
    COMMS_EXPAND(COMMS_DEFINE_FIELD_ENUM(__VA_ARGS__)) \
    FUNC_AUTO_REF_RETURN(asVariant, decltype(comms::field::toFieldBase(*this))) { \
        auto& var = comms::field::toFieldBase(*this); \
        using Var = typename std::decay<decltype(var)>::type; \
        static_assert(std::tuple_size<typename Var::Members>::value == FieldIdx_numOfValues, \
            "Invalid number of names for variant field"); \
        return var; \
    }\
    FUNC_AUTO_REF_RETURN_CONST(asVariant, decltype(comms::field::toFieldBase(*this))) { \
        auto& var = comms::field::toFieldBase(*this); \
        using Var = typename std::decay<decltype(var)>::type; \
        static_assert(std::tuple_size<typename Var::Members>::value == FieldIdx_numOfValues, \
            "Invalid number of names for variant field"); \
        return var; \
    } \
    COMMS_DO_VARIANT_MEM_ACC_FUNC(asVariant(), __VA_ARGS__)


}  // namespace field

}  // namespace comms

