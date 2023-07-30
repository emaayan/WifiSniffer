package org.wifisniff;

import com.fazecast.jSerialComm.SerialPort;

import java.io.Closeable;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.util.ArrayList;
import java.util.List;

import static com.fazecast.jSerialComm.SerialPort.NO_PARITY;

public class SerialCtrl implements Closeable {

    public static List<String> getAllPorts() {
        final SerialPort[] commPorts = SerialPort.getCommPorts();
        final List<String> comms = new ArrayList<>(commPorts.length);
        for (SerialPort commPort : commPorts) {
            comms.add(commPort.getSystemPortName());
        }
        return comms;
    }


    private final SerialPort serialPort;

    public SerialCtrl(String port, int speed) {
        serialPort = SerialPort.getCommPort(port);
        serialPort.setBaudRate(speed);
        serialPort.setNumDataBits(8);
        serialPort.setNumStopBits(1);
        serialPort.setParity(NO_PARITY);
        serialPort.setComPortTimeouts(SerialPort.TIMEOUT_READ_BLOCKING, 2000, 0);
    }

    public void open() {
        serialPort.openPort();
    }

    public int send(String cmd) {
        final byte[] bytes = cmd.getBytes();
        final int i = serialPort.writeBytes(bytes, bytes.length);
        return i;
    }

    public ByteBuffer read(int length) {
        final byte[] bytes = new byte[length];
        final ByteBuffer byteBuffer;
        final int i = serialPort.readBytes(bytes, bytes.length);
        if (i > 0) {
            byteBuffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
        } else {
            byteBuffer = ByteBuffer.allocate(0);
        }
        return byteBuffer;
    }

    @Override
    public void close() {
        serialPort.closePort();
    }

}
