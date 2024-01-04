#pragma once

#include <memory>

struct ID3D11Buffer;

namespace pbl
{

class GenericBuffer {
public:
  using bufferflags_t = int;
  enum BufferFlags : bufferflags_t {
	TYPE_CONSTANT = 1<<0,
	TYPE_VERTEX   = 1<<1,
	TYPE_INDEX    = 1<<2,
	FLAG_MUTABLE  = 1<<3,

	BUFFER_VERTEX   = TYPE_VERTEX,
	BUFFER_INDEX    = TYPE_INDEX,
	BUFFER_CONSTANT = TYPE_CONSTANT | FLAG_MUTABLE,
	BUFFER_INSTANCE = TYPE_VERTEX | FLAG_MUTABLE,
  };

  template<class ContentType>
  static std::shared_ptr<GenericBuffer> make_buffer(bufferflags_t flags, const ContentType &initialData) {
	return std::make_shared<GenericBuffer>(sizeof(ContentType), flags, &initialData);
  }

  template<class ContentType>
  static std::shared_ptr<GenericBuffer> make_buffer(bufferflags_t flags) {
	return std::make_shared<GenericBuffer>(sizeof(ContentType), flags);
  }

  GenericBuffer() = default;
  GenericBuffer(size_t contentSize, bufferflags_t flags, const void *initialData=nullptr);
  ~GenericBuffer();

  GenericBuffer(const GenericBuffer &) = delete;
  GenericBuffer &operator=(const GenericBuffer &) = delete;
  GenericBuffer(GenericBuffer &&moved) noexcept : m_buffer(std::exchange(moved.m_buffer, nullptr)) {}
  GenericBuffer &operator=(GenericBuffer &&moved) noexcept { GenericBuffer{ std::move(moved) }.swap(*this); return *this; }

  void swap(GenericBuffer &other) noexcept {
	std::swap(other.m_buffer, m_buffer);
  }

  template<class ContentType>
  void setData(const ContentType &content) { setRawData(reinterpret_cast<const void *>(&content)); }
  void setRawData(const void *content);
  ID3D11Buffer *const &getRawBuffer() const { return m_buffer; }

private:
  ID3D11Buffer *m_buffer = nullptr;
};

}