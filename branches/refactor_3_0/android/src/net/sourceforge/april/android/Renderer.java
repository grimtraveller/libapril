package net.sourceforge.april.android;

// version 2.5

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import android.app.Activity;

public class Renderer implements android.opengl.GLSurfaceView.Renderer
{
	public void onSurfaceCreated(GL10 gl, EGLConfig config)
	{
		NativeInterface.onSurfaceCreated();
		if (!NativeInterface.Running)
		{
			NativeInterface.setVariables(NativeInterface.DataPath, NativeInterface.ArchivePath);
			String args[] = {NativeInterface.ApkPath}; // adding argv[0]
			NativeInterface.init(args);
			NativeInterface.Running = true;
		}
	}
	
	public void onSurfaceChanged(GL10 gl, int w, int h)
	{
	}
	
	public void onDrawFrame(GL10 gl)
	{
		if (!NativeInterface.render())
		{
			NativeInterface.Activity.finish();
		}
	}
	
}