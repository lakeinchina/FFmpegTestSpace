package me.lake.encoder_yuv2h264;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.util.Log;
import android.view.SurfaceHolder;

import java.io.IOException;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;

import me.lake.ffmpeg.FFmpeg;
import me.lake.ffmpeg.FFmpegClient;

/**
 * Created by lake on 16-5-24.
 */
public class RESVideoClient {
    private static int videoWidth = 1280;
    private static int videoHeight = 720;
    private static int videoBitrate = 800 * 1024;
    private final Object syncOp = new Object();
    private Camera camera;
    private int cameraNum;
    private int currentCameraIndex;
    private boolean isStreaming;
    int previewBufferSize;
    private SurfaceHolder surfaceHolder;
    FFmpegClient ffmpegClient;
    long startTime;
    byte[] tmpyuv420data;

    public RESVideoClient(SurfaceHolder surfaceHolder) {
        this.surfaceHolder = surfaceHolder;
        cameraNum = Camera.getNumberOfCameras();
        currentCameraIndex = Camera.CameraInfo.CAMERA_FACING_BACK;
        isStreaming = false;
        ffmpegClient = new FFmpegClient();
        startTime = 0;
    }

    public boolean prepare() {
        synchronized (syncOp) {
            currentCameraIndex = Camera.CameraInfo.CAMERA_FACING_BACK;
            if (null == (camera = createCamera(currentCameraIndex))) {
                Log.e("RESVideoClient", "can not open camera");
                return false;
            }
            Camera.Parameters parameters = camera.getParameters();
            parameters.setPreviewSize(videoWidth, videoHeight);
            previewBufferSize = videoWidth * videoHeight * 3 / 2;
            parameters.setPreviewFormat(ImageFormat.NV21);
            selectCameraFpsRange(parameters);
            configCamera(camera, parameters);
            camera.addCallbackBuffer(new byte[previewBufferSize]);
            camera.addCallbackBuffer(new byte[previewBufferSize]);
            tmpyuv420data = new byte[previewBufferSize];
            return true;
        }
    }

    private Camera createCamera(int cameraId) {
        try {
            camera = Camera.open(cameraId);
        } catch (SecurityException e) {
            LogTools.trace("no permission", e);
            return null;
        } catch (Exception e) {
            LogTools.trace("camera.open()failed", e);
            return null;
        }
        return camera;
    }

    private boolean startVideo() {
        camera.setPreviewCallbackWithBuffer(new Camera.PreviewCallback() {
            @Override
            public void onPreviewFrame(byte[] data, Camera camera) {
                synchronized (syncOp) {
                    if (data != null) {
                        FFmpeg.H264.NV21TOYUV420P(data, tmpyuv420data, videoWidth * videoHeight);
                        ffmpegClient.encode(tmpyuv420data, startTime++);
                    }
                    camera.addCallbackBuffer(data);
                }
            }
        });
        try {
            camera.setPreviewDisplay(surfaceHolder);
        } catch (IOException e) {
            LogTools.trace(e);
            camera.release();
            return false;
        }
        camera.startPreview();
        return true;
    }


    public boolean start() {
        synchronized (syncOp) {
            if (!isStreaming) {
                if (!startVideo()) {
                    LogTools.e("RESVideoClient,start(),failed");
                    return false;
                }
                ffmpegClient.start(videoWidth, videoHeight, videoBitrate);
            }
            isStreaming = true;
            return true;
        }
    }

    public boolean stop() {
        synchronized (syncOp) {
            if (isStreaming) {
                camera.stopPreview();
                ffmpegClient.flush();
                ffmpegClient.end();
            }
            isStreaming = false;
            return true;
        }
    }


    public boolean destroy() {
        synchronized (syncOp) {
            camera.release();
            camera = null;
            return true;
        }
    }


    public boolean setZoomByPercent(float targetPercent) {
        synchronized (syncOp) {
            targetPercent = Math.min(Math.max(0f, targetPercent), 1f);
            Camera.Parameters p = camera.getParameters();
            p.setZoom((int) (p.getMaxZoom() * targetPercent));
            camera.setParameters(p);
            return true;
        }
    }

    public static int targetFps = 30000;

    public static boolean configCamera(Camera camera, Camera.Parameters parameters) {
        parameters.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
        List<String> focusModes = parameters.getSupportedFocusModes();
        if (focusModes != null) {
            if (focusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO)) {
                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_VIDEO);
            } else if (focusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
            } else if (focusModes.contains(Camera.Parameters.FOCUS_MODE_FIXED)) {
                parameters.setFocusMode(Camera.Parameters.FOCUS_MODE_FIXED);
            }
        }
        try {
            camera.setParameters(parameters);
        } catch (Exception e) {
            camera.release();
            return false;
        }
        return true;
    }

    public static void selectCameraFpsRange(Camera.Parameters parameters) {
        List<int[]> fpsRanges = parameters.getSupportedPreviewFpsRange();
        Collections.sort(fpsRanges, new Comparator<int[]>() {
            @Override
            public int compare(int[] lhs, int[] rhs) {
                int r = Math.abs(lhs[0] - targetFps) + Math.abs(lhs[1] - targetFps);
                int l = Math.abs(rhs[0] - targetFps) + Math.abs(rhs[1] - targetFps);
                if (r > l) {
                    return 1;
                } else if (r < l) {
                    return -1;
                } else {
                    return 0;
                }
            }
        });
        parameters.setPreviewFpsRange(fpsRanges.get(0)[0], fpsRanges.get(0)[1]);
    }

}
