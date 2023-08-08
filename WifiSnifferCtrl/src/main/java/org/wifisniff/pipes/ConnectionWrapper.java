package org.wifisniff.pipes;

import com.sun.jna.platform.win32.Kernel32;
import com.sun.jna.ptr.IntByReference;

import java.io.Closeable;
import java.io.IOException;
import java.nio.ByteBuffer;

public interface ConnectionWrapper extends Closeable {

    public static ConnectionWrapper  NULL_WRAPPER=new ConnectionWrapper() {
        @Override
        public boolean connect() throws PipeException {
            return true;
        }

        @Override
        public int write(ByteBuffer byteBuffer) throws PipeException {
            return byteBuffer.capacity();
        }

        @Override
        public void close() throws IOException {
            System.out.println("CLOSED");
        }
    };
    public boolean connect() throws PipeException;

    public int write(ByteBuffer byteBuffer) throws PipeException;


}