package org.wifisniff.capture;

import java.io.IOException;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.Optional;
import java.util.function.Function;
import java.util.function.Supplier;

public abstract class AbstractPacket {

    private static final byte[] HEX_ARRAY = "0123456789ABCDEF".getBytes(StandardCharsets.US_ASCII);

    public static String bytesToHex(ByteBuffer byteBuffer) {
        return bytesToHex(byteBuffer.array());
    }

    public static String bytesToHex(byte[] bytes) {
        final byte[] hexChars = new byte[bytes.length * 2];
        for (int j = 0; j < bytes.length; j++) {
            int v = bytes[j] & 0xFF;
            hexChars[j * 2] = HEX_ARRAY[v >>> 4];
            hexChars[j * 2 + 1] = HEX_ARRAY[v & 0x0F];
        }
        return new String(hexChars, StandardCharsets.UTF_8);
    }

    public static byte[] hexStringToByteArray(String s) {
        final int len = s.length();
        final byte[] data = new byte[len / 2];
        for (int i = 0; i < len; i += 2) {
            final byte b = (byte) ((Character.digit(s.charAt(i), 16) << 4) + Character.digit(s.charAt(i + 1), 16));
            data[i / 2] = b;
        }
        return data;
    }

    private static int bytesToIntBE(byte[] bytes) {
        return bytesToIntBE(bytes, 0, bytes.length);
    }

    private static int bytesToIntLE(byte[] bytes, int offset, int length) {
        int i = offset + length;
        int value = bytes[--i];
        while (--i >= offset) {
            value <<= 8;
            value |= bytes[i] & 0xFF;
        }
        return value;
    }


    private static int bytesToIntBE(byte[] bytes, int offset, int length) {
        int endOffset = offset + length;
        int value = bytes[offset];
        while (++offset < endOffset) {
            value <<= 8;
            value |= bytes[offset] & 0xFF;
        }
        return value;
    }

    public void write(OutputStream outputStream) throws IOException {
        final byte[] array = this.byteBuffer.array();//  order.array();
        bytesToIntLE(array,0,array.length);
        outputStream.write(array, 0, array.length);
        System.out.println(bytesToHex(array));
    }

    static <T extends AbstractPacket> Optional<T> get(ByteBuffer byteBuffer, int size, Function<ByteBuffer, T> tSupplier) {
        if (byteBuffer.remaining() >= size) {
            final T apply = tSupplier.apply(byteBuffer);
             //System.out.println(apply.toHexString()+ " "+apply);
            return Optional.of(apply);
        } else {
            return Optional.empty();
        }
    }

    private final ByteBuffer byteBuffer;

    public AbstractPacket(ByteBuffer byteBuffer) {
        this.byteBuffer = byteBuffer;
    }

    public ByteBuffer getByteBuffer() {
        return byteBuffer;
    }

    public String toHexString() {
        final ByteBuffer byteBuffer1 = getByteBuffer();
        final byte[] array = byteBuffer1.array();
        return bytesToHex(array);
    }
}
