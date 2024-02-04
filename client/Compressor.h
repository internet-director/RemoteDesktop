#pragma once
#include <memory>
#include <type_traits>
#include "stdafx.h"

template<auto F>
using function_t = std::decay_t<decltype(F)>;

namespace compressor_details
{
	NTSTATUS WINAPI RtlDecompressBuffer(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, PULONG);
	NTSTATUS WINAPI RtlCompressBuffer(USHORT, PUCHAR, ULONG, PUCHAR, ULONG, ULONG, PULONG, PVOID);
	NTSTATUS WINAPI RtlGetCompressionWorkSpaceSize(USHORT, PULONG, PULONG);
}

class Compressor
{
	function_t<compressor_details::RtlCompressBuffer> fRtlCompressBuffer{ nullptr };
	function_t<compressor_details::RtlDecompressBuffer> fRtlDecompressBuffer{ nullptr };
	function_t<compressor_details::RtlGetCompressionWorkSpaceSize> fRtlGetCompressionWorkSpaceSize{ nullptr };
	
	bool _inited{ false };
	std::unique_ptr<BYTE> workspace;

public:
	Compressor();

	bool inited() const noexcept { return _inited; }
	bool init_workspace();
	size_t compress(const void* src, size_t src_len, void* dst, size_t dst_len);
	size_t decompress(const void* src, size_t src_len, void* dst, size_t dst_len);
};

