#include "Compressor.h"

Compressor::Compressor()
{
	HMODULE ntdll;
	if ((ntdll = GetModuleHandleA("ntdll.dll")) == NULL)
	{
		return;
	}

	fRtlCompressBuffer =
		reinterpret_cast< function_t< compressor_details::RtlCompressBuffer > >(GetProcAddress(ntdll, "RtlCompressBuffer"));
	fRtlDecompressBuffer =
		reinterpret_cast< function_t< compressor_details::RtlDecompressBuffer > >(GetProcAddress(ntdll, "RtlDecompressBuffer"));
	fRtlGetCompressionWorkSpaceSize = reinterpret_cast< function_t< compressor_details::RtlGetCompressionWorkSpaceSize > >(
		GetProcAddress(ntdll, "RtlGetCompressionWorkSpaceSize"));

	_inited = fRtlCompressBuffer && fRtlDecompressBuffer && fRtlGetCompressionWorkSpaceSize;
}

bool Compressor::init_workspace()
{
	if (!_inited)
	{
		return false;
	}

	ULONG ws_size, fs_size;
	NTSTATUS status = fRtlGetCompressionWorkSpaceSize(COMPRESSION_FORMAT_LZNT1, &ws_size, &fs_size);

	if (!NT_SUCCESS(status))
	{
		return false;
	}

	workspace.reset(new BYTE[ws_size]);
	return true;
}

size_t Compressor::compress(const void* src, size_t src_len, void* dst, size_t dst_len)
{
	if (!_inited)
	{
		return -1;
	}

	ULONG c_size;
	NTSTATUS status = fRtlCompressBuffer(
		COMPRESSION_FORMAT_LZNT1,
		(PUCHAR)src,
		src_len,
		(PUCHAR)dst,
		dst_len,
		4096,
		&c_size,
		workspace.get());

	if (!NT_SUCCESS(status))
	{
		return -1;
	}

	return c_size;
}

size_t Compressor::decompress(const void* src, size_t src_len, void* dst, size_t dst_len)
{
	if (!_inited)
	{
		return -1;
	}

	ULONG c_size;
	NTSTATUS status = fRtlDecompressBuffer(COMPRESSION_FORMAT_LZNT1, (PUCHAR)dst, dst_len, (PUCHAR)src, src_len, &c_size);

	if (!NT_SUCCESS(status))
	{
		return -1;
	}

	return c_size;
}
