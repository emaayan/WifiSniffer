package org.wifisniff.capture;

import java.nio.ByteBuffer;
import java.util.Optional;

public class PayloadHeader extends AbstractPacket {

    public static final int SIZE = 16;

    private final int timeStamp;
    private final int microSeconds;
    private final int incl_len;
    private final int orig_len;

    public static Optional<PayloadHeader> get(ByteBuffer byteBuffer) {
        return get(byteBuffer, SIZE, PayloadHeader::new);
    }

    public PayloadHeader(ByteBuffer payloadHeader) {
        super(payloadHeader);
        timeStamp = payloadHeader.getInt();
        microSeconds = payloadHeader.getInt();
        incl_len = payloadHeader.getInt();
        orig_len = payloadHeader.getInt();
    }


    public int getTimeStamp() {
        return timeStamp;
    }

    public int getMicroSeconds() {
        return microSeconds;
    }

    public int getIncl_len() {
        return incl_len;
    }

    public int getOrig_len() {
        return orig_len;
    }

    @Override
    public String toString() {
        return "PayloadHeader{" +
                "timeStamp=" + timeStamp +
                ", microSeconds=" + microSeconds +
                ", incl_len=" + incl_len +
                ", orig_len=" + orig_len +
                '}';
    }
}
