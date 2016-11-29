package me.lake.ffmpeg;

/**
 * Created by lake on 23/11/16.
 * Decoder_any2pcm project.
 */
@SuppressWarnings("all")
public class FFmpeg {
    static {
        System.loadLibrary("x264");
        System.loadLibrary("ffmpeg");
        System.loadLibrary("yuv2h264");
    }

    public static native NativeMediaData open(String filePath);

    public static native long decodeNextFrame(long pointer,DecodeCallback decodeCallback);

    public static native long seekTo(long pointer,long timeSec);

    public static native long close(long pointer);

    public interface DecodeCallback {
        void onDecodedData(byte[] decodedData, long pts);
    }
}
