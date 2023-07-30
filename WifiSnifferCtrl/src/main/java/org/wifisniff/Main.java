package org.wifisniff;


import org.wifisniff.capture.CaptureHeader;
import org.wifisniff.capture.PayloadHeader;
import org.wifisniff.capture.PayloadPacket;
import org.wifisniff.pipes.ConnectionWrapper;
import org.wifisniff.pipes.NamedPipeWrapper;
import org.wifisniff.pipes.PipeException;
import org.wifisniff.pipes.TCPWrapper;

import java.io.IOException;
import java.nio.ByteBuffer;
import java.util.Optional;
import java.util.function.Consumer;
import java.util.function.Predicate;


public class Main {

    public static void main(String[] args) throws IOException, PipeException {

        String pipeName = "\\\\.\\pipe\\wireshark";
//        pipeName="TCP@127.0.0.1:19000";
//        pipeName="TCP@127.0.0.1";
        final int speed = 250000;//912600;
        final String port = "COM7";

        final ConnectionWrapper namedPipeWrapper = new NamedPipeWrapper(pipeName);
        launchWireshark(pipeName);
        namedPipeWrapper.connect();

        final SerialCtrl serialCtrl = new SerialCtrl(port, speed);
        serialCtrl.open();

        serialCtrl.send("S1");


        final ByteBuffer captureHeaderBytes = serialCtrl.read(CaptureHeader.SIZE);
        final Optional<CaptureHeader> captureHeader = CaptureHeader.get(captureHeaderBytes);
        captureHeader.ifPresent(captureHeader1 -> {
            final int magic = captureHeader1.getMagicNum();

//        serialCtrl.send("F2000CCC");

            if (magic == CaptureHeader.MAGIC) {
                namedPipeWrapper.write(captureHeader1.getByteBuffer());
                while (true) {

                    final ByteBuffer payloadHeaderBytes = serialCtrl.read(PayloadHeader.SIZE);
                    final Optional<PayloadHeader> header = PayloadHeader.get(payloadHeaderBytes);
                    header.ifPresent(payloadHeader -> {
                        final int origLen = payloadHeader.getOrig_len();
                        if (origLen >= 0) {//to be used to parse other stuff //maybe as payload type for logs

                            namedPipeWrapper.write(payloadHeader.getByteBuffer());
                            final int inclLen = payloadHeader.getIncl_len();

                            final ByteBuffer payload = serialCtrl.read(inclLen);
                            final Optional<PayloadPacket> payloadPacket1 = PayloadPacket.get(payload);
                            payloadPacket1.ifPresent(payloadPacket -> {
                                namedPipeWrapper.write(payloadPacket.getByteBuffer());
                                System.out.printf("GOT %s ,%s\n", payloadHeader, payloadPacket);
                            });

                        } else {
                            final int inclLen = payloadHeader.getIncl_len();
                            final ByteBuffer payload = serialCtrl.read(inclLen);
                            final byte[] array = payload.array();
                            final String s = new String(array);
                            System.out.println("Got message " + s);
                        }
                    });
                }


            } else {
                System.out.println("ERROR");
            }
        });

    }


    private String pipeName = "\\\\.\\pipe\\wireshark";

    private int speed = 250000;//912600;
    private String port;// = "COM7";
    private String wiresharkExec = "C:\\Program Files\\Wireshark\\Wireshark.exe";

    public Main() {

    }

    public String getPipeName() {
        return pipeName;
    }

    public void setPipeName(String pipeName) {
        this.pipeName = pipeName;
    }

    public int getSpeed() {
        return speed;
    }

    public void setSpeed(int speed) {
        this.speed = speed;
    }

    public String getPort() {
        return port;
    }

    public void setPort(String port) {
        this.port = port;
    }

    public Main(String pipeName, int speed, String port) {
        this.pipeName = pipeName;
        this.speed = speed;
        this.port = port;
    }

    private static String processDetails(ProcessHandle process) {
        return String.format("%8d %8s %10s %26s %-40s",
                process.pid(),
                text(process.parent().map(ProcessHandle::pid)),
                text(process.info().user()),
                text(process.info().startInstant()),
                text(process.info().arguments()));
    }

    private static String text(Optional<?> optional) {
        return optional.map(Object::toString).orElse("");
    }

    private static void launchWireshark(String pipeName) throws IOException {
//https://github.com/espressif/esp-idf/blob/master/examples/protocols/sockets/tcp_server/main/tcp_server.c
//TODO: capture the process handl
//TODO: check to see if we can use TCP settings!!! https://wiki.wireshark.org/CaptureSetup/Pipes#tcp-socket
        //https://randomnerdtutorials.com/esp32-access-point-ap-web-server/
        final String wiresharkExec = "C:\\Program Files\\Wireshark\\Wireshark.exe";
        final ProcessBuilder processBuilder = new ProcessBuilder(wiresharkExec, "-i" + pipeName, "-k");
        final Process start = processBuilder.start();


    }

    public static void main_ol8d(String[] args) throws InterruptedException {
        Sniffer sniffer = new Sniffer();

    }

}