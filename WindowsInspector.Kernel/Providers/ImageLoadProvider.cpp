#include <WindowsInspector.Kernel/Error.hpp>
#include <WindowsInspector.Kernel/Mem.hpp>
#include <WindowsInspector.Kernel/Common.hpp>
#include <WindowsInspector.Kernel/DataItemList.hpp>
#include <WindowsInspector.Kernel/EventList.hpp>
#include "ImageLoadProvider.hpp"

void OnImageLoadNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo);

NTSTATUS InitializeImageLoadProvider()
{
    return PsSetLoadImageNotifyRoutine(OnImageLoadNotify);
}

void ReleaseImageLoadProvider()
{
    PsRemoveLoadImageNotifyRoutine(OnImageLoadNotify);
}

const WCHAR Unknown[] = L"(Unknown)";
ULONG UnknownSize = sizeof(Unknown);

void OnImageLoadNotify(_In_opt_ PUNICODE_STRING FullImageName, _In_ HANDLE ProcessId, _In_ PIMAGE_INFO ImageInfo)
{
    ULONG fullImageNameLen = 0;
    PCWSTR fullImageName;

    if (FullImageName)
    {
        fullImageName = FullImageName->Buffer;
        fullImageNameLen = FullImageName->Length;
    }
    else
    {
        fullImageName = Unknown;
        fullImageNameLen = UnknownSize;
    }

    auto* newItem = Mem::Allocate<ListItem<ImageLoadEvent>>(fullImageNameLen);

    if (newItem == NULL)
    {
        KdPrintMessage("Could not allocate load image info");
        return;
    }

    ImageLoadEvent& info = newItem->Item;

    RtlCopyMemory(info.ImageFileName, fullImageName, fullImageNameLen);
    info.ImageFileNameLength = fullImageNameLen / 2;
    info.ImageSize = ImageInfo->ImageSize;
    info.ProcessId = HandleToUlong(ProcessId);
    info.LoadAddress = (ULONG_PTR)ImageInfo->ImageBase;
    info.ThreadId = HandleToUlong(PsGetCurrentThreadId());
    info.Size = sizeof(ImageLoadEvent) + (USHORT)fullImageNameLen;
    info.Type = EventType::ImageLoad;
    KeQuerySystemTimePrecise(&info.Time);

    InsertIntoList(&newItem->Entry, EventType::ImageLoad);

}