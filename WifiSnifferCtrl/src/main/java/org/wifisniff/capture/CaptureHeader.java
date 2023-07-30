package org.wifisniff.capture;

import java.nio.ByteBuffer;
import java.util.Optional;

public class CaptureHeader extends AbstractPacket {


    public final static int SIZE = 24;
    public final static int MAGIC = 0xa1b2c3d4;
    private final int magicNum;
    private final short majorV;
    private final short minorV;
    private final int timezone;
    private final int sigfigs;
    private final int snapLen;
    private final int networkType;

    public static Optional<CaptureHeader> get(ByteBuffer byteBuffer) {
        return get(byteBuffer, SIZE, CaptureHeader::new);
    }

    public CaptureHeader(ByteBuffer byteBuffer) {
        super(byteBuffer);
        magicNum = byteBuffer.getInt();
        majorV = byteBuffer.getShort();
        minorV = byteBuffer.getShort();
        timezone = byteBuffer.getInt();
        sigfigs = byteBuffer.getInt();
        snapLen = byteBuffer.getInt();
        networkType = byteBuffer.getInt();
    }

    @Override
    public String toString() {
        return "CaptureHeader{" +
                "magicNum=" + String.format("%X", magicNum) +
                ", majorV=" + majorV +
                ", minorV=" + minorV +
                ", timezone=" + timezone +
                ", sigfigs=" + sigfigs +
                ", snapLen=" + snapLen +
                ", networkType=" + networkType +
                '}';
    }

    public int getMagicNum() {
        return magicNum;
    }

    public short getMajorV() {
        return majorV;
    }

    public short getMinorV() {
        return minorV;
    }

    public int getTimezone() {
        return timezone;
    }

    public int getSnapLen() {
        return snapLen;
    }

    public int getNetworkType() {
        return networkType;
    }
}
