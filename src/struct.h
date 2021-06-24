#pragma once

#include "types.h"
#include "utils.h"
#include <map>

namespace bpftrace {

struct Bitfield
{
  bool operator==(const Bitfield &other) const;
  bool operator!=(const Bitfield &other) const;

  // Read `read_bytes` bytes starting from this field's offset
  size_t read_bytes;
  // Then rshift the resulting value by `access_rshift` to get field value
  size_t access_rshift;
  // Then logical AND `mask` to mask out everything but this bitfield
  uint64_t mask;
};

struct Field
{
  std::string name;
  SizedType type;
  ssize_t offset;

  bool is_bitfield;
  Bitfield bitfield;

  // Used for tracepoint __data_loc's
  //
  // If true, this field is a 32 bit integer whose value encodes information on
  // where to find the actual data. The first 2 bytes is the size of the data.
  // The last 2 bytes is the offset from the start of the tracepoint struct
  // where the data begins.
  bool is_data_loc = false;

  bool operator==(const Field &rhs) const
  {
    return name == rhs.name && type == rhs.type && offset == rhs.offset &&
           is_bitfield == rhs.is_bitfield && bitfield == rhs.bitfield &&
           is_data_loc == rhs.is_data_loc;
  }
};

using Fields = std::vector<Field>;

struct Struct
{
  int size; // in bytes
  int align = 1; // in bytes, used for tuples only
  bool padded = false;
  Fields fields;

  explicit Struct(int size) : size(size)
  {
  }

  bool HasField(const std::string &name) const;
  const Field &GetField(const std::string &name) const;
  void AddField(const std::string &field_name,
                const SizedType &type,
                ssize_t offset,
                bool is_bitfield,
                const Bitfield &bitfield,
                bool is_data_loc);

  static std::unique_ptr<Struct> CreateTuple(std::vector<SizedType> fields);
  void Dump(std::ostream &os);

  bool operator==(const Struct &rhs) const
  {
    return size == rhs.size && align == rhs.align && padded == rhs.padded &&
           fields == rhs.fields;
  }
};

std::ostream &operator<<(std::ostream &os, const Fields &t);

} // namespace bpftrace

namespace std {
template <>
struct hash<bpftrace::Struct>
{
  size_t operator()(const bpftrace::Struct &s) const
  {
    size_t hash = std::hash<int>()(s.size);
    for (auto &field : s.fields)
      bpftrace::hash_combine(hash, field.type);
    return hash;
  }
};
} // namespace std

namespace bpftrace {

class StructManager
{
public:
  void Add(const std::string &name, size_t size);
  Struct *AddTuple(std::vector<SizedType> fields);
  Struct *Lookup(const std::string &name) const;
  Struct *LookupOrAdd(const std::string &name, size_t size);
  bool Has(const std::string &name) const;

private:
  std::map<std::string, std::unique_ptr<Struct>> struct_map_;
  std::vector<std::unique_ptr<Struct>> tuples_;
};

} // namespace bpftrace
