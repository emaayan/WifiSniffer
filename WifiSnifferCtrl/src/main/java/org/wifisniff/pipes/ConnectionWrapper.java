package org.wifisniff.pipes;

import com.sun.jna.platform.win32.Kernel32;
import com.sun.jna.ptr.IntByReference;

import java.io.Closeable;
import java.nio.ByteBuffer;

public interface ConnectionWrapper extends Closeable {

    public boolean connect() throws PipeException;

    public int write(ByteBuffer byteBuffer) throws PipeException;


}