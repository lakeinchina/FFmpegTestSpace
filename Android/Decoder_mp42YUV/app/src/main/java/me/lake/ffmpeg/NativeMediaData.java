package me.lake.ffmpeg;

import java.lang.reflect.Field;
import java.lang.reflect.Modifier;

/**
 * Created by lake on 22/11/16.
 * Decoder_mp42YUV project.
 */
public class NativeMediaData {
    public long nativePointer;
    public int width;
    public int height;
    public long duration;
    public float frameRate;

    public NativeMediaData() {

    }

    public void setNativePointer(long nativePointer) {
        this.nativePointer = nativePointer;
    }

    public void setWidth(int width) {
        this.width = width;
    }

    public void setHeight(int height) {
        this.height = height;
    }

    public void setDuration(long duration) {
        this.duration = duration;
    }

    public void setFrameRate(float frameRate) {
        this.frameRate = frameRate;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append("ResParameter:");
        Field[] fields = this.getClass().getDeclaredFields();
        for (Field field : fields) {
            if (Modifier.isStatic(field.getModifiers())) {
                continue;
            }
            field.setAccessible(true);
            try {
                sb.append(field.getName());
                sb.append('=');
                sb.append(field.get(this));
                sb.append(';');
            } catch (IllegalAccessException e) {
            }
        }
        return super.toString()+";"+sb.toString();
    }
}
