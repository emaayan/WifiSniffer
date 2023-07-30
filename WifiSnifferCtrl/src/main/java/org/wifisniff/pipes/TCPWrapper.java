package org.wifisniff.pipes;

import javax.net.ServerSocketFactory;
import java.io.IOException;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.ByteBuffer;

public class TCPWrapper implements ConnectionWrapper {


    private final static ServerSocketFactory SERVER_SOCKET_FACTORY = ServerSocketFactory.getDefault();
    private ServerSocket serverSocket;
    private Socket accept;

    public TCPWrapper(String name) {
        try {
            serverSocket = SERVER_SOCKET_FACTORY.createServerSocket(19000);
        } catch (IOException e) {
            throw new PipeException(e.getMessage());
        }
    }

    @Override
    public boolean connect() throws PipeException {
        try {
            accept = serverSocket.accept();
            return true;
        } catch (IOException e) {
            throw new PipeException(e.getMessage());
        }

    }

    @Override
    public int write(ByteBuffer byteBuffer) throws PipeException {
        try {
            final OutputStream outputStream = accept.getOutputStream();
            outputStream.write(byteBuffer.array());
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
        return 0;
    }

    @Override
    public void close() throws IOException {
        serverSocket.close();
    }
}
