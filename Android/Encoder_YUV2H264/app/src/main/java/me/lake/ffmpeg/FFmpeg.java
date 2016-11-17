package me.lake.ffmpeg;

/**
 * Created by lake on 16/11/16.
 * Librestreaming project.
 */
public class FFmpeg {
    public static class H264 {
        static {
            System.loadLibrary("x264");
            System.loadLibrary("ffmpeg");
            System.loadLibrary("yuv2h264");
        }
        static public native void NV21TOYUV420P(byte[] src, byte[] dst, int YSize);

        public static native long create(int width, int height, int bitrate);

        public static native int encode(long pointer, byte[] pixdata, long pts, EncodeCallback encodeCallback);

        public static native int flush(long pointer, EncodeCallback encodeCallback);

        public static native int destroy(long pointer);
    }

    public interface EncodeCallback {
        void onEncodedData(byte[] encodedData, long pts, long dts);
    }
}
