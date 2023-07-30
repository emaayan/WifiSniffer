package org.wifisniff.capture;

import org.wifisniff.SerialCtrl;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.Optional;

public class PayloadPacket extends AbstractPacket {

    public final static int SIZE = 28;

    private final short frameControl;
    private final short duration;

    private final byte[] da = new byte[6];
    private final byte[] sa = new byte[6];
    private final byte[] bssid = new byte[6];
    private final int seqControl;
    private final byte[] data;

    public static Optional<PayloadPacket> get(ByteBuffer byteBuffer) {
        return get(byteBuffer, SIZE, PayloadPacket::new);
    }

    public PayloadPacket(ByteBuffer payload) {
        super(payload);
        frameControl = payload.getShort();
        duration = payload.getShort();
        payload.get(da);
        payload.get(sa);
        payload.get(bssid);
        seqControl = payload.getShort() & 0xFF;
        final int remaining = payload.remaining();
        final byte[] data = new byte[remaining];
        payload.get(data);
        this.data = data;
    }

    public short getFrameControl() {
        return frameControl;
    }

    public short getDuration() {
        return duration;
    }

    public byte[] getDa() {
        return da;
    }

    public byte[] getSa() {
        return sa;
    }

    public byte[] getBssid() {
        return bssid;
    }

    public int getSeqControl() {
        return seqControl;
    }

    public byte[] getData() {
        return data;
    }

    @Override
    public String toString() {
        return "PayloadPacket{" +
                "frameControl=" + frameControl +
                ", duration=" + duration +
                ", da=" +bytesToHex(da) +
                ", sa=" + bytesToHex(sa) +
                ", bssid=" + bytesToHex(bssid) +
                ", seqControl=" + seqControl +
                '}';
    }
}

