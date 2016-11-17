package me.lake.ffmpeg;

import android.util.Log;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;

/**
 * Created by lake on 17/11/16.
 * Librestreaming project.
 */
public class FFmpegClient {
    private long pointer;
    FFmpeg.EncodeCallback encodeCallback = new FFmpeg.EncodeCallback() {
        @Override
        public void onEncodedData(byte[] encodedData, long pts, long dts) {
            Log.e("aa", "onEncodedData" + encodedData + "pts" + pts + "dts" + dts);
            try {
                fos.write(encodedData);
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
    };
    FileOutputStream fos;

    public void start(int w, int h, int bitrate) {
        pointer = FFmpeg.H264.create(w, h, bitrate);
        try {
            fos = new FileOutputStream("/sdcard/test.h264");
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
    }

    public int encode(byte[] pixdata, long pts) {
        Log.e("aa", "pts" + pts);
        if (pointer != 0) {
            return FFmpeg.H264.encode(pointer, pixdata, pts, encodeCallback);
        } else {
            return -1;
        }
    }

    public int flush() {
        return FFmpeg.H264.flush(pointer, encodeCallback);
    }

    public int end() {
        try {
            fos.flush();
            fos.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
        int ret = FFmpeg.H264.destroy(pointer);
        pointer = 0;
        return ret;
    }

}
