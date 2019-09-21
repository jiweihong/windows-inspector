#include <WindowsInspector.Kernel/Common.hpp>
#include <WindowsInspector.Kernel/Debug.hpp>
#include <WindowsInspector.Kernel/EventBuffer.hpp>
#include "ProcessProvider.hpp"

NTSTATUS InitializeProcessProvider();
void ReleaseProcessProvider();

void OnProcessNotify(_Inout_ PEPROCESS, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo);

NTSTATUS InitializeProcessProvider()
{
    return PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, FALSE);
}

void ReleaseProcessProvider()
{
    PsSetCreateProcessNotifyRoutineEx(OnProcessNotify, TRUE);
}

void OnProcessStart(_In_ HANDLE ProcessId, _Inout_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    if (CreateInfo->CommandLine == NULL)
    {
        D_ERROR("Failed to log ProcessCreateInfo: CommandLine is NULL");
        return;
    }
    
    ProcessCreateEvent* event;
    NTSTATUS status = AllocateBufferEvent(&event, sizeof(ProcessCreateEvent) + CreateInfo->CommandLine->Length);
    
    if (!NT_SUCCESS(status))
    {
        D_ERROR_STATUS("Failed to allocate memory for ProcessCreateInfo", status);
        return;
    }

    event->Type = EventType::ProcessCreate;
    event->Size = sizeof(ProcessCreateEvent) + CreateInfo->CommandLine->Length;
    KeQuerySystemTimePrecise(&event->Time);
    event->NewProcessId = HandleToUlong(ProcessId);
    event->ParentProcessId = HandleToUlong(CreateInfo->ParentProcessId);
    event->ProcessId = HandleToUlong(CreateInfo->CreatingThreadId.UniqueProcess);
    event->ThreadId = HandleToUlong(CreateInfo->CreatingThreadId.UniqueThread);
    event->CommandLine.Size = CreateInfo->CommandLine->Length;

    RtlCopyMemory(
        event->GetProcessCommandLine(),
        CreateInfo->CommandLine->Buffer,
        CreateInfo->CommandLine->Length
    );

    SendBufferEvent(event);
}

void OnProcessExit(_In_ HANDLE ProcessId)
{
    ProcessExitEvent* event;
    NTSTATUS status = AllocateBufferEvent(&event, sizeof(ProcessExitEvent*));

    if (!NT_SUCCESS(status))
    {
        D_ERROR("Failed to allocate memory for ProcessExitInfo");
        return;
    }

    event->Type = EventType::ProcessExit;
    event->Size = sizeof(ProcessExitEvent);
    KeQuerySystemTimePrecise(&event->Time);
    event->ProcessId = HandleToUlong(ProcessId);
    event->ThreadId = HandleToUlong(PsGetCurrentThreadId());
    SendBufferEvent(event);
}

void OnProcessNotify(_Inout_ PEPROCESS ProcessObject, _In_ HANDLE ProcessId, _Inout_opt_ PPS_CREATE_NOTIFY_INFO CreateInfo)
{
    UNREFERENCED_PARAMETER(ProcessObject);

    if (CreateInfo)
    {
        OnProcessStart(ProcessId, CreateInfo);
    }
    else
    {
        OnProcessExit(ProcessId);
    }
}