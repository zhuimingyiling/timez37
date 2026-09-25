package com.timez.chess;

import android.app.Application;
import android.content.Context;
import com.google.android.material.color.DynamicColors;

public class ChessApp extends Application {
    private static Context appContext;

    /** Get the application context. */
    public static Context getContext() {
        return appContext;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        appContext = this;
        DynamicColors.applyToActivitiesIfAvailable(this);
    }
}