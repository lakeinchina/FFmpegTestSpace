package me.lake.decoder_mp42yuv;

import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;

import me.lake.ffmpeg.FFmpeg;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        FFmpeg.create();
    }
}
