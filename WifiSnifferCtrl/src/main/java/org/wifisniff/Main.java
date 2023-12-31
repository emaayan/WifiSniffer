package org.wifisniff;


import org.locationtech.jts.geom.*;
import org.locationtech.jts.index.kdtree.KdTree;
import org.locationtech.jts.index.strtree.GeometryItemDistance;
import org.locationtech.jts.index.strtree.ItemBoundable;
import org.locationtech.jts.index.strtree.ItemDistance;
import org.locationtech.jts.index.strtree.STRtree;
import org.wifisniff.capture.AbstractPacket;
import org.wifisniff.capture.CaptureHeader;
import org.wifisniff.capture.PayloadHeader;
import org.wifisniff.capture.PayloadPacket;
import org.wifisniff.pipes.ConnectionWrapper;
import org.wifisniff.pipes.NamedPipeWrapper;
import org.wifisniff.pipes.PipeException;
import org.wifisniff.pipes.TCPWrapper;

import javax.net.ServerSocketFactory;
import javax.net.SocketFactory;
import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.SocketTimeoutException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.Files;
import java.nio.file.Paths;
import java.sql.SQLOutput;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Optional;
import java.util.concurrent.CountDownLatch;
import java.util.function.BiConsumer;
import java.util.function.Consumer;
import java.util.function.Function;
import java.util.function.Predicate;


public class Main {

    //    public static ByteBuffer read(int length, Consumer<ByteBuffer>bb ) {
//        final byte[] bytes = new byte[length];
//        final int i =bb.accept(byteBuffer);
//        if (i > 0) {
//            byteBuffer = ByteBuffer.wrap(bytes).order(ByteOrder.LITTLE_ENDIAN);
//        } else {
//            byteBuffer = ByteBuffer.allocate(0);
//        }
//        return byteBuffer;
//    }


    public static void main_8(String[] args) throws IOException {

        String localhost = "192.168.9.217";

        final int port1 = 19000;
        while (true) {
            try (final Socket socket = SocketFactory.getDefault().createSocket(localhost, port1)) {
                final OutputStream outputStream = socket.getOutputStream();
                outputStream.write(0);
//            socket.setKeepAlive(true);
                final int timeout = 2000;

                socket.setSoTimeout(timeout);
                final InputStream inputStream = socket.getInputStream();
                //final OutputStream out = Files.newOutputStream(Paths.get("test.pcap"));

                final ByteBuffer bb = getByteBuffer(inputStream, CaptureHeader.SIZE);
                final Optional<CaptureHeader> captureHeader = CaptureHeader.get(bb);
                captureHeader.ifPresent(captureHeader1 -> {
                    //     System.out.println(captureHeader1);
                    while (socket.isConnected()) {
                        try {
                            final ByteBuffer byteBuffer = getByteBuffer(inputStream, PayloadHeader.SIZE);
                            PayloadHeader.get(byteBuffer).ifPresent(payloadHeader -> {
                                System.out.println(payloadHeader);
                                try {
                                    //         payloadHeader.write(out);
                                    final ByteBuffer byteBuffer1 = getByteBuffer(inputStream, payloadHeader.getIncl_len());
                                    PayloadPacket.get(byteBuffer1).ifPresent(payloadPacket -> {
                                        final byte[] data = payloadPacket.getData();
                                        System.out.println(payloadPacket + " " + AbstractPacket.bytesToHex(data));

                                    });
                                } catch (IOException e) {
                                    throw new RuntimeException(e);
                                }
                            });

                        } catch (SocketTimeoutException e) {
                            e.printStackTrace();
                        } catch (IOException e) {
                            throw new RuntimeException(e);
                        }

                    }
                });
            }

        }

    }

    private static ByteBuffer getByteBuffer(InputStream inputStream, int size) throws IOException {
        final ByteBuffer allocate = ByteBuffer.allocate(size);//.order(ByteOrder.LITTLE_ENDIAN);
        final byte[] array = allocate.array();
        final int read = inputStream.read(array);
        return allocate;
    }

    public static class CaptureStream {

        private final SerialCtrl serialCtrl;

        public CaptureStream(SerialCtrl serialCtrl) {
            this.serialCtrl = serialCtrl;
        }

        public void stream() {
            serialCtrl.open();


            serialCtrl.send("S1");

            //while (true) {
            final ByteBuffer captureHeaderBytes = serialCtrl.read(CaptureHeader.SIZE);
            final Optional<CaptureHeader> captureHeader = CaptureHeader.get(captureHeaderBytes);
            captureHeader.ifPresent(captureHeader1 -> {
                final int magic = captureHeader1.getMagicNum();
                if (magic == CaptureHeader.MAGIC) {
                    //    namedPipeWrapper.write(captureHeader1.getByteBuffer());
                    while (true) {

                        final ByteBuffer payloadHeaderBytes = serialCtrl.read(PayloadHeader.SIZE);
                        final Optional<PayloadHeader> header = PayloadHeader.get(payloadHeaderBytes);
                        header.ifPresent(payloadHeader -> {
                            final int origLen = payloadHeader.getOrig_len();

                            //       namedPipeWrapper.write(payloadHeader.getByteBuffer());
                            final int inclLen = payloadHeader.getIncl_len();

                            final ByteBuffer payload = serialCtrl.read(inclLen);
                            final Optional<PayloadPacket> payloadPacket1 = PayloadPacket.get(payload);
                            payloadPacket1.ifPresent(payloadPacket -> {
                                //           namedPipeWrapper.write(payloadPacket.getByteBuffer());
                                System.out.printf("GOT %s ,%s\n", payloadHeader, payloadPacket);
                            });


                        });
                    }


                } else {
                    final String x = AbstractPacket.bytesToHex(captureHeaderBytes);
                    System.out.println(x);
//                final byte[] bytes = AbstractPacket.hexStringToByteArray(captureHeaderBytes);

                    //System.out.println(new String(captureHeaderBytes.array()));
                }
            });
        }
    }

    public static class Point{
        public int x;
        public int y;

        public Point(int x, int y) {
            this.x = x;
            this.y = y;
        }

        @Override
        public String toString() {
            return "Point{" +
                    "x=" + x +
                    ", y=" + y +
                    '}';
        }
    }
//    public static Point findClosestGridPoint(Point[] grid, double x, double y) {
//        Point closestPoint = null;
//        double minDistance = Double.MAX_VALUE;
//
//        for (Point point : grid) {
//            double distance = Math.sqrt(Math.pow(point.x - x, 2) + Math.pow(point.y - y, 2));
//            if (distance < minDistance) {
//                minDistance = distance;
//                closestPoint = point;
//            }
//        }
//
//        return closestPoint;
//    }
    public static void main(String[] args) {
//         Point grid[]={new Point(0,0),new Point(0,2),new Point(0,4)
//                      ,new Point(2,0),new Point(2,2),new Point(2,4)
//                      ,new Point(4,0),new Point(4,2),new Point(4,4)
//         };
//        System.out.println(findClosestGridPoint(grid,2.4,0.4));
//        System.out.println(findClosestGridPoint(grid,3.6,0.4));

        List<Coordinate> gridPoints = new ArrayList<>();
        gridPoints.add(new Coordinate(0, 0));
        gridPoints.add(new Coordinate(0, 2));
        gridPoints.add(new Coordinate(0, 4));
        gridPoints.add(new Coordinate(2, 0));
        gridPoints.add(new Coordinate(2, 2));
        gridPoints.add(new Coordinate(2, 4));
        gridPoints.add(new Coordinate(4, 0));
        gridPoints.add(new Coordinate(4, 2));
        gridPoints.add(new Coordinate(4, 4));

        final Envelope env = new Envelope(gridPoints.get(0),gridPoints.get(gridPoints.size()-1));

        final STRtree stRtree=new STRtree();
        gridPoints.forEach(gridPoint -> {
            final Envelope itemEnv = new Envelope(gridPoint);
            stRtree.insert(itemEnv,gridPoint);
        });

        final Coordinate target = new Coordinate(3.4, 2.4);

        final Object o = stRtree.nearestNeighbour(env, target, (item1, item2) -> {
            final Envelope c1 = (Envelope) item1.getBounds();
            final Coordinate c2 = (Coordinate) item2.getItem();
            return c1.centre().distance(c2);
        });
        System.out.println(o);


//        int grid[][]={
//                 {2,4,6,8,10}
//                ,{4,6,8,10,12}
//                ,{6,8,10,12,14}
//        };
//        for (int x = 0; x < grid.length; x++) {
//            for (int y = 0; y <grid[x].length; y++) {
//                System.out.println(grid[x][y]);
//            }
//        }
    }

    public static void main_2sds32(String[] args) {

        final int speed = 115200;
        final String port = "COM7";
        final SerialCtrl serialCtrl = new SerialCtrl(port, speed);
        serialCtrl.open();




        while (true) {
            ByteBuffer byteBuffer=ByteBuffer.allocate(1000);
            ByteBuffer bb = null;
            while (bb == null || bb.capacity() == 0) {
                bb = serialCtrl.read(1);
            }
            if(bb.capacity()>0){
                byteBuffer.put(bb);
                final String s = new String(bb.array());
                System.out.print(s);
            }
//            final byte b = bb.get(0);
//            if ((b >= 'A') && (b <= 'z')) {
//                final ByteBuffer allocate = ByteBuffer.allocate(1000);
//                ByteBuffer read = null;
//                while (read == null || read.capacity() == 0) {
//                    read = serialCtrl.read(1);
//                    if (read.capacity() > 0) {
//                        if (read.get(0) == '\n') {
//                            break;
//                        }
//                        allocate.put(read);
//                    }
//
//                }
//
//                final String s = new String(allocate.array());
//                System.out.print(s);
//
//            }
        }


    }

    public static void main_oo(String[] args) throws InterruptedException {

        final int speed = 115200;
        final String port = "COM7";
        final SerialCtrl serialCtrl = new SerialCtrl(port, speed);
        serialCtrl.open();

        serialCtrl.send("S1");

        //while (true) {
        final ByteBuffer captureHeaderBytes = serialCtrl.read(CaptureHeader.SIZE);
        final Optional<CaptureHeader> captureHeader = CaptureHeader.get(captureHeaderBytes);
        captureHeader.ifPresent(captureHeader1 -> {
            final int magic = captureHeader1.getMagicNum();

//        serialCtrl.send("F2000CCC");

            if (magic == CaptureHeader.MAGIC) {
                //    namedPipeWrapper.write(captureHeader1.getByteBuffer());
                while (true) {

                    final ByteBuffer payloadHeaderBytes = serialCtrl.read(PayloadHeader.SIZE);
                    final Optional<PayloadHeader> header = PayloadHeader.get(payloadHeaderBytes);
                    header.ifPresent(payloadHeader -> {
                        final int origLen = payloadHeader.getOrig_len();

                        //       namedPipeWrapper.write(payloadHeader.getByteBuffer());
                        final int inclLen = payloadHeader.getIncl_len();

                        final ByteBuffer payload = serialCtrl.read(inclLen);
                        final Optional<PayloadPacket> payloadPacket1 = PayloadPacket.get(payload);
                        payloadPacket1.ifPresent(payloadPacket -> {
                            //           namedPipeWrapper.write(payloadPacket.getByteBuffer());
                            System.out.printf("GOT %s ,%s\n", payloadHeader, payloadPacket);
                        });


                    });
                }


            } else {
                final String x = AbstractPacket.bytesToHex(captureHeaderBytes);
                System.out.println(x);
//                final byte[] bytes = AbstractPacket.hexStringToByteArray(captureHeaderBytes);

                //System.out.println(new String(captureHeaderBytes.array()));
            }
        });
        //  }
        //new CountDownLatch(1).await();
    }

    public static void main_232(String[] args) throws IOException, PipeException {

        String pipeName = "\\\\.\\pipe\\wireshark";
//        pipeName="TCP@127.0.0.1:19000";
//        pipeName="TCP@127.0.0.1";
        final int speed = 115200;//912600;//115200;// 250000;//912600;
        final String port = "COM6";

        //  final ConnectionWrapper namedPipeWrapper = ConnectionWrapper.NULL_WRAPPER;// new NamedPipeWrapper(pipeName);
        final ConnectionWrapper namedPipeWrapper = new NamedPipeWrapper(pipeName);
        //  launchWireshark(pipeName);
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
                        if (origLen > 0) {//to be used to parse other stuff //maybe as payload type for logs

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

    public Main(String pipeName, int speed, String port) {
        this.pipeName = pipeName;
        this.speed = speed;
        this.port = port;
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
        //TODO: capture the process handl
        //https://randomnerdtutorials.com/esp32-access-point-ap-web-server/
        final String wiresharkExec = "C:\\Program Files\\Wireshark\\Wireshark.exe";
        final ProcessBuilder processBuilder = new ProcessBuilder(wiresharkExec, "-i" + pipeName, "-k");
        final Process start = processBuilder.start();


    }

    public static void main_ol8d(String[] args) throws InterruptedException {
        Sniffer sniffer = new Sniffer();

    }

}