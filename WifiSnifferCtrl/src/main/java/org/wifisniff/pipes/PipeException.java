package org.wifisniff.pipes;

import com.sun.jna.LastErrorException;

public class PipeException extends LastErrorException {


    public PipeException(String msg) {
        super(msg);
    }

    public PipeException(int code, String msg) {
        super(code,msg);
    }

}
