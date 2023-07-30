package org.wifisniff.pipes;

import com.sun.jna.LastErrorException;
import com.sun.jna.Native;
import com.sun.jna.platform.win32.Kernel32;
import com.sun.jna.platform.win32.WinBase;
import com.sun.jna.platform.win32.WinError;
import com.sun.jna.platform.win32.WinNT;
import com.sun.jna.ptr.IntByReference;

import java.io.Closeable;
import java.nio.ByteBuffer;
import java.util.function.Predicate;

public class NamedPipeWrapper implements ConnectionWrapper {

    private final WinNT.HANDLE handle;

    private final int nOutBufferSize = 65536;
    private final int nInBufferSize = 65536;
    private final int nDefaultTimeOut = 300;
    private final int nMaxInstances = 1;

    public NamedPipeWrapper(String name) throws PipeException {
        final WinNT.HANDLE hNamedPipe = Kernel32.INSTANCE.CreateNamedPipe(name, WinBase.PIPE_ACCESS_OUTBOUND, WinBase.PIPE_TYPE_MESSAGE | WinBase.PIPE_WAIT, nMaxInstances, nOutBufferSize, nInBufferSize, nDefaultTimeOut, null);
        getLastError(hNamedPipe, o -> o == WinBase.INVALID_HANDLE_VALUE);
        this.handle = hNamedPipe;
    }

    private void getLastError(boolean result) throws PipeException {
        getLastError(result, aBoolean -> !aBoolean);
    }

    private String getErrorDescription(int errorCode) {
        final String errMsgFmt = "Error %d: %s - %s";
        switch (errorCode) {
            case WinError.ERROR_BAD_PIPE:
                return String.format(errMsgFmt, errorCode, "ERROR_BAD_PIPE", "The pipe state is invalid");
            case WinError.ERROR_PIPE_BUSY:
                return String.format(errMsgFmt, errorCode, "ERROR_PIPE_BUSY", "All pipe instances are busy");
            case WinError.ERROR_NO_DATA:
                return String.format(errMsgFmt, errorCode, "ERROR_NO_DATA", "The pipe is being closed");
            case WinError.ERROR_PIPE_NOT_CONNECTED:
                return String.format(errMsgFmt, errorCode, "ERROR_PIPE_NOT_CONNECTED", "No process is on the other end of the pipe");
            default:
                return String.format(errMsgFmt, errorCode, "Unknown", "Unknown Error");
        }
    }

    private <T> void getLastError(T context, Predicate<T> errorCondition) throws PipeException {
        final boolean test = errorCondition.test(context);
        if (test) {
            final int errorCode = Native.getLastError();// Kernel32.INSTANCE.GetLastError();
            if (errorCode != WinError.NO_ERROR) {
                final String errorDescription = getErrorDescription(errorCode);
                throw new PipeException(errorCode,errorDescription);
            }
        }
    }

    public boolean connect() throws PipeException {
        final boolean b = Kernel32.INSTANCE.ConnectNamedPipe(handle, null);
        getLastError(b);
        return b;
    }

    public int write(ByteBuffer byteBuffer) throws PipeException {
        final byte[] array = byteBuffer.array();
        final IntByReference lpNumberOfBytesWritten = new IntByReference(0);
        final boolean b = Kernel32.INSTANCE.WriteFile(handle, array, array.length, lpNumberOfBytesWritten, null);
        getLastError(b);
        final int value = lpNumberOfBytesWritten.getValue();
        return value;
    }

    @Override
    public void close() {
        final boolean b = Kernel32.INSTANCE.CloseHandle(handle);
        getLastError(b);
    }
}
