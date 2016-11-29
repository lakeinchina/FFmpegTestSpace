package me.lake.decoder_any2pcm;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;

import me.lake.ffmpeg.FFmpeg;
import me.lake.ffmpeg.NativeMediaData;

public class MainActivity extends AppCompatActivity {
    Button btn_play;
    boolean seeked = false;
    boolean isDestroyed=false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        isDestroyed=false;
        btn_play = (Button) findViewById(R.id.btn_play);
        btn_play.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View view) {
                new Thread() {
                    @Override
                    public void run() {
                        final PcmPlayer pcmPlayer = new PcmPlayer();
                        pcmPlayer.prepare();
                        pcmPlayer.start();
                        NativeMediaData nativeMediaData = FFmpeg.open("/sdcard/taudio.aac");
                        FFmpeg.DecodeCallback decodeCallback = new FFmpeg.DecodeCallback() {
                            @Override
                            public void onDecodedData(byte[] decodedData, long pts) {
                                pcmPlayer.write(decodedData);
                                Log.e("aa", "datalen=" + decodedData.length + "pts=" + pts);
                            }
                        };
                        while (!isDestroyed) {
                            long ret;
                            if ((ret = FFmpeg.decodeNextFrame(nativeMediaData.nativePointer, decodeCallback)) < 0) {
                                break;
                            }
                            if (ret == 10 & !seeked) {
                                FFmpeg.seekTo(nativeMediaData.nativePointer, 20);
                                seeked = true;
                            }
                        }
                        pcmPlayer.stop();
                        pcmPlayer.destroy();
                    }
                }.start();


            }
        });
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        isDestroyed=true;
    }
}
