#include "generic_buffer.h"

#include <stdexcept>

#include "directxlib.h"
#include "engine/windowsengine.h"
#include "utils/debug.h"

namespace pbl
{

GenericBuffer::GenericBuffer(size_t contentSize, bufferflags_t flags, const void *initialData)
  : m_buffer(nullptr)
{
  // https://learn.microsoft.com/en-us/windows/win32/api/d3d11/nf-d3d11-id3d11device-createbuffer#remarks
  PBL_ASSERT(!(flags & BufferFlags::TYPE_CONSTANT) || contentSize % 16 == 0, "Attempted to create a buffer with size not a multiple of 16");
  PBL_ASSERT(flags & BufferFlags::FLAG_MUTABLE || initialData != nullptr, "Attempted to create an immutable buffer without data");
  D3D11_BUFFER_DESC bufDesc;
  D3D11_SUBRESOURCE_DATA initData;
  ZeroMemory(&bufDesc, sizeof(bufDesc));
  ZeroMemory(&initData, sizeof(initData));
  bufDesc.Usage = (flags & BufferFlags::FLAG_MUTABLE) ? D3D11_USAGE_DEFAULT : D3D11_USAGE_IMMUTABLE;
  bufDesc.ByteWidth = static_cast<UINT>(contentSize);
  bufDesc.BindFlags |= (flags & BufferFlags::TYPE_CONSTANT) ? D3D11_BIND_CONSTANT_BUFFER : 0;
  bufDesc.BindFlags |= (flags & BufferFlags::TYPE_VERTEX  ) ? D3D11_BIND_VERTEX_BUFFER   : 0;
  bufDesc.BindFlags |= (flags & BufferFlags::TYPE_INDEX   ) ? D3D11_BIND_INDEX_BUFFER    : 0;
  bufDesc.CPUAccessFlags = 0;
  initData.pSysMem = initialData;

  PBL_ASSERT((~flags & BufferFlags::TYPE_CONSTANT) || (flags & BufferFlags::FLAG_MUTABLE), "Created a non-mutable constant buffer");
  PBL_ASSERT(bufDesc.BindFlags, "Created a typeless constant buffer");

  DXTry(
    WindowsEngine::d3ddevice().CreateBuffer(
      &bufDesc,
      initialData != nullptr ? &initData : nullptr,
      &m_buffer),
    "Could not create a gpu buffer");
}

GenericBuffer::~GenericBuffer()
{
  DXRelease(m_buffer);
}

void GenericBuffer::setRawData(const void *content)
{
  WindowsEngine::d3dcontext().UpdateSubresource(m_buffer, 0, nullptr, content, 0, 0);
}

}
