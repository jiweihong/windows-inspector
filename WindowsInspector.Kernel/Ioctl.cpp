#include "Ioctl.hpp"
#include <WindowsInspector.Kernel/Providers/Providers.hpp>
#include <WindowsInspector.Kernel/Debug.hpp>
#include "EventBuffer.hpp"
#include "Common.hpp"


NTSTATUS InitializeIoctlHandlers()
{
    return ZeroEventBuffer();
}

NTSTATUS InspectorListen(PIRP Irp)
{
    CircularBuffer** UserBaseAddressPtr = (CircularBuffer**)Irp->AssociatedIrp.SystemBuffer;
    
    NTSTATUS status = InitializeEventBuffer(UserBaseAddressPtr);

    if (!NT_SUCCESS(status))
    {
        D_ERROR_STATUS("Could not allocate event buffer", status);
    }
    else
    {
        // Return address to user mode
        Irp->IoStatus.Information = sizeof(ULONG_PTR);
    }

    return STATUS_SUCCESS;
}