#include "Serialization.h"

namespace rc {
namespace detail {

template <typename T, typename Iterator, typename>
Iterator serialize(T value, Iterator output) {
  using UInt = typename std::make_unsigned<T>::type;
  constexpr auto nbytes = std::numeric_limits<UInt>::digits / 8;

  auto uvalue = static_cast<UInt>(value);
  auto it = output;
  for (std::size_t i = 0; i < nbytes; i++) {
    *it = static_cast<std::uint8_t>(uvalue);
    it++;
    uvalue = uvalue >> 8;
  }

  return it;
}

template <typename T, typename Iterator, typename>
Iterator deserialize(Iterator begin, Iterator end, T &out) {
  using UInt = typename std::make_unsigned<T>::type;
  constexpr auto nbytes = std::numeric_limits<UInt>::digits / 8;

  UInt uvalue = 0;
  auto it = begin;
  for (std::size_t i = 0; i < nbytes; i++) {
    if (it == end) {
      throw SerializationException("Unexpected end of input");
    }

    uvalue |= static_cast<UInt>(*it & 0xFF) << (i * 8);
    it++;
  }

  out = static_cast<T>(uvalue);
  return it;
}

template <typename InputIterator, typename OutputIterator>
OutputIterator serializeN(InputIterator in, std::size_t n, OutputIterator out) {
  auto oit = out;
  auto iit = in;
  for (std::size_t i = 0; i < n; i++) {
    oit = serialize(*iit, oit);
    iit++;
  }

  return oit;
}

template <typename T, typename InputIterator, typename OutputIterator>
InputIterator deserializeN(InputIterator begin,
                           InputIterator end,
                           std::size_t n,
                           OutputIterator out) {
  auto iit = begin;
  auto oit = out;
  for (std::size_t i = 0; i < n; i++) {
    T value;
    iit = deserialize(iit, end, value);
    *oit = std::move(value);
    oit++;
  }

  return iit;
}

template <typename T, typename Iterator>
Iterator serializeCompact(T value, Iterator output) {
  static_assert(std::is_integral<T>::value, "T must be integral");

  using UInt = typename std::make_unsigned<T>::type;
  auto uvalue = static_cast<UInt>(value);
  do {
    auto x = uvalue & 0x7F;
    uvalue = uvalue >> 7;
    *output = static_cast<std::uint8_t>((uvalue == 0) ? x : (x | 0x80));
    output++;
  } while (uvalue != 0);

  return output;
}

template <typename T, typename Iterator>
Iterator deserializeCompact(Iterator begin, Iterator end, T &output) {
  static_assert(std::is_integral<T>::value, "T must be integral");
  using UInt = typename std::make_unsigned<T>::type;

  UInt uvalue = 0;
  int nbits = 0;
  for (auto it = begin; it != end; it++) {
    auto x = static_cast<std::uint8_t>(*it);
    uvalue |= static_cast<UInt>(x & 0x7F) << nbits;
    nbits += 7;
    if ((x & 0x80) == 0) {
      output = static_cast<T>(uvalue);
      return ++it;
    }
  }

  throw SerializationException("Unexpected end of input");
}

template <typename InputIterator, typename OutputIterator>
OutputIterator
serializeCompact(InputIterator begin, InputIterator end, OutputIterator output) {
  const std::uint64_t numElements = std::distance(begin, end);
  auto oit = serializeCompact(numElements, output);
  for (auto it = begin; it != end; it++) {
    oit = serializeCompact(*it, oit);
  }

  return oit;
}

template <typename T, typename InputIterator, typename OutputIterator>
std::pair<InputIterator, OutputIterator> deserializeCompact(
    InputIterator begin, InputIterator end, OutputIterator output) {
  std::uint64_t numElements;
  auto iit = deserializeCompact(begin, end, numElements);
  auto oit = output;
  for (std::uint64_t i = 0; i < numElements; i++) {
    T value;
    iit = deserializeCompact(iit, end, value);
    *oit = value;
    oit++;
  }

  return std::make_pair(iit, oit);
}

} // namespace detail
} // namespace rc