#pragma once

#include "common.h"

namespace HolographicAppForOpenGLES1
{
    class SimpleRenderer
    {
    public:
        SimpleRenderer(bool holographic, Windows::Graphics::Holographic::HolographicSpace ^pHolographicSpace);
        ~SimpleRenderer();
		void setEGL(EGLDisplay pEGLDisplay, EGLSurface pEGLSurface, EGLContext pEGLContext);
		void buildWindow();
		void initialize();
        void Draw();
        void UpdateWindowSize(GLsizei width, GLsizei height);

    private:
        GLuint mProgram;
        GLsizei mWindowWidth;
        GLsizei mWindowHeight;

        GLint mPositionAttribLocation;
        GLint mColorAttribLocation;

        GLint mModelUniformLocation;
        GLint mViewUniformLocation;
        GLint mProjUniformLocation;
        GLint mRtvIndexAttribLocation;

        GLuint mVertexPositionBuffer;
        GLuint mVertexColorBuffer;
        GLuint mIndexBuffer;
        GLuint mRenderTargetArrayIndices;

        int mDrawCount;
        bool mIsHolographic;

		EGLDisplay mEGLDisplay;
		EGLSurface mEGLSurface;
		EGLContext mEGLContext;

		EventRegistrationToken mCameraAddedEventToken;
		Windows::Graphics::Holographic::HolographicSpace ^mHolographicSpace;
    };
}
