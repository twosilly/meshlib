/****************************************************************************
* MeshLab                                                           o o     *
* A versatile mesh processing toolbox                             o     o   *
*                                                                _   O  _   *
* Copyright(C) 2005                                                \/)\/    *
* Visual Computing Lab                                            /\/|      *
* ISTI - Italian National Research Council                           |      *
*                                                                    \      *
* All rights reserved.                                                      *
*                                                                           *
* This program is free software; you can redistribute it and/or modify      *
* it under the terms of the GNU General Public License as published by      *
* the Free Software Foundation; either version 2 of the License, or         *
* (at your option) any later version.                                       *
*                                                                           *
* This program is distributed in the hope that it will be useful,           *
* but WITHOUT ANY WARRANTY; without even the implied warranty of            *
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
* GNU General Public License (http://www.gnu.org/licenses/gpl.txt)          *
* for more details.                                                         *
*                                                                           *
****************************************************************************/

#include "decorate_raster_proj.h"
#include <wrap/gl/shot.h>
#include <common/pluginmanager.h>
#include <meshlab/glarea.h>
#include <vcg/math/matrix44.h>




void DecorateRasterProjPlugin::MeshDrawer::drawShadow()
{
    if( !m_Mesh->visible )
        return;

    if( m_VBO.IsInstantiated() )
    {
        glPushAttrib( GL_TRANSFORM_BIT );
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glMultMatrix(m_Mesh->cm.Tr);

        m_VBO.Normal.Disable();
        m_VBO.Bind();
        m_VBO.DrawElements( GL_TRIANGLES, 0, 3*m_Mesh->cm.fn );
        m_VBO.Unbind();
        m_VBO.Normal.Enable();

        glPopMatrix();
        glPopAttrib();
    }
    else
        m_Mesh->Render( vcg::GLW::DMFlat, vcg::GLW::CMNone, vcg::GLW::TMNone );
}


void DecorateRasterProjPlugin::MeshDrawer::draw()
{
    if( !m_Mesh->visible )
        return;

    if( m_VBO.IsInstantiated() )
    {
        glPushAttrib( GL_TRANSFORM_BIT );
        glMatrixMode( GL_MODELVIEW );
        glPushMatrix();
        glMultMatrix(m_Mesh->cm.Tr);

        m_VBO.Bind();
        m_VBO.DrawElements( GL_TRIANGLES, 0, 3*m_Mesh->cm.fn );
        m_VBO.Unbind();

        glPopMatrix();
        glPopAttrib();
    }
    else
        m_Mesh->Render( vcg::GLW::DMSmooth, vcg::GLW::CMNone, vcg::GLW::TMNone );
}


void DecorateRasterProjPlugin::MeshDrawer::update( bool useVBO )
{
    // Initialize the VBO if required.
    if( useVBO && m_Mesh->visible )
    {
        if( !m_VBO.IsInstantiated() )
        {
            m_VBO.Create();

            // Transfer of vertex positions on GPU.
            CMeshO &meshData = m_Mesh->cm;
            vcg::Point3f *vertBuffer = new vcg::Point3f [ 2*meshData.vn ];
            for( int i=0, n=0; i<meshData.vn; ++i )
            {
                vertBuffer[n++] = meshData.vert[i].P();
                vertBuffer[n++] = meshData.vert[i].N();
            }

            m_VBO.LoadData( GL_STATIC_DRAW_ARB, vertBuffer, 2*meshData.vn );
            m_VBO.Vertex.SetPointer( 2*sizeof(vcg::Point3f), 0 );
            m_VBO.Normal.SetPointer( 2*sizeof(vcg::Point3f), sizeof(vcg::Point3f) );
            delete [] vertBuffer;

            // Transfer of face indices on GPU.
            unsigned int *indexBuffer = new unsigned int [ 3*meshData.fn ];
            for( int i=0, n=0; i<meshData.fn; ++i )
            {
                indexBuffer[n++] = meshData.face[i].V(0) - &meshData.vert[0];
                indexBuffer[n++] = meshData.face[i].V(1) - &meshData.vert[0];
                indexBuffer[n++] = meshData.face[i].V(2) - &meshData.vert[0];
            }

            m_VBO.LoadIndices( GL_STATIC_DRAW_ARB, indexBuffer, 3*meshData.fn );
            delete [] indexBuffer;
        }
    }
    else
        m_VBO.Release();
}




bool DecorateRasterProjPlugin::s_AreVBOSupported;

    
DecorateRasterProjPlugin::DecorateRasterProjPlugin() :
    m_CurrentRaster(NULL),
    m_CurrentMesh(NULL)
{
    typeList << DP_PROJECT_RASTER;

    foreach( FilterIDType tt, types() )
        actionList << new QAction(decorationName(tt), this);

    foreach( QAction *ap, actionList )
        ap->setCheckable(true);
}

        
DecorateRasterProjPlugin::~DecorateRasterProjPlugin()
{
    glPushAttrib( GL_ALL_ATTRIB_BITS );

    m_Scene.clear();
    m_ShadowMapShader.Release();
    m_DepthTexture.Release();
    m_ColorTexture.Release();

    glPopAttrib();
}


QString DecorateRasterProjPlugin::decorationInfo( FilterIDType id ) const
{
    switch( id )
    {
        case DP_PROJECT_RASTER: return tr("Project the current raster onto the 3D mesh");
        default: assert(0); return QString();
    }  
}


QString DecorateRasterProjPlugin::decorationName( FilterIDType id ) const
{
    switch( id )
    {
        case DP_PROJECT_RASTER: return tr("Rasters-to-geometry reprojection");
        default: assert(0); return QString();
    }
}


int DecorateRasterProjPlugin::getDecorationClass( QAction *act ) const
{
    switch( ID(act) )
    {
        case DP_PROJECT_RASTER: return PerDocument|PostRendering;
        default: assert(0); return Generic;
    }
}


void DecorateRasterProjPlugin::initGlobalParameterSet( QAction *act, RichParameterSet &par )
{
    switch( ID(act) )
    {
        case DP_PROJECT_RASTER:
        {
            par.addParam( new RichDynamicFloat( "MeshLab::Decoration::ProjRasterAlpha",
                                                1.0f,
                                                0.0f,
                                                1.0f,
                                                "Transparency",
                                                "Transparency" ) );

            par.addParam( new RichBool( "MeshLab::Decoration::ProjRasterLighting",
                                        true,
                                        "Apply lighting",
                                        "Apply lighting" ) );

            par.addParam( new RichBool( "MeshLab::Decoration::ProjRasterUseVBO",
                                        false,
                                        "Use VBO",
                                        "Use VBO" ) );

            par.addParam( new RichBool( "MeshLab::Decoration::ProjRasterOnAllMeshes",
                                        false,
                                        "Project on all meshes",
                                        "Project the current raster on all meshes instead of only on the current one" ) );
            break;
        }
        default: assert(0);
    }
}		


void DecorateRasterProjPlugin::updateCurrentMesh( MeshDocument &m,
                                                  RichParameterSet &par )
{
    if( par.getBool("MeshLab::Decoration::ProjRasterOnAllMeshes") )
    {
        QMap<int,MeshDrawer> tmpScene = m_Scene;
        m_Scene.clear();

        foreach( MeshModel *md, m.meshList )
        {
            QMap<int,MeshDrawer>::iterator t = tmpScene.find( md->id() );
            if( t != tmpScene.end() )
                m_Scene[ t.key() ] = t.value();
            else
                m_Scene[ md->id() ] = MeshDrawer( md );
        }
    }
    else
    {
        if( m_CurrentMesh && m.mm()==m_CurrentMesh->mm() )
            return;

        m_Scene.clear();
        m_CurrentMesh = &( m_Scene[m.mm()->id()] = MeshDrawer(m.mm()) );
    }


    bool areVBORequired = par.getBool( "MeshLab::Decoration::ProjRasterUseVBO" );
    if( areVBORequired && !s_AreVBOSupported )
    {
        par.setValue( "MeshLab::Decoration::ProjRasterUseVBO", BoolValue(false) );
        areVBORequired = false;
    }

    m_SceneBox.SetNull();
    for( QMap<int,MeshDrawer>::iterator m=m_Scene.begin(); m!=m_Scene.end(); ++m )
    {
        m_SceneBox.Add( m->mm()->cm.Tr, m->mm()->cm.bbox );
        m->update( areVBORequired );
    }
}


void DecorateRasterProjPlugin::updateShadowProjectionMatrix()
{
    // Recover the near and far clipping planes by considering the bounding box of the current mesh
    // in the camera space of the current raster.
    float zNear, zFar;
    GlShot< vcg::Shot<float> >::GetNearFarPlanes( m_CurrentRaster->shot, m_SceneBox, zNear, zFar );
    if( zNear < 0.0001f )
        zNear = 0.1f;
    if( zFar < zNear )
        zFar = zNear + 1000.0f;


    // Recover the view frustum of the current raster.
    float l, r, b, t, focal;
    m_CurrentRaster->shot.Intrinsics.GetFrustum( l, r, b, t, focal );


    // Compute from the frustum values the camera projection matrix.
    const float normFactor = zNear / focal;
    l *= normFactor;
    r *= normFactor;
    b *= normFactor;
    t *= normFactor;

    m_RasterProj.SetZero();
    m_RasterProj[0][0] = 2.0f*zNear / (r-l);
    m_RasterProj[2][0] = (r+l) / (r-l);
    m_RasterProj[1][1] = 2.0f*zNear / (t-b);
    m_RasterProj[2][1] = (t+b) / (t-b);
    m_RasterProj[2][2] = (zNear+zFar) / (zNear-zFar);
    m_RasterProj[3][2] = 2.0f*zNear*zFar / (zNear-zFar);
    m_RasterProj[2][3] = -1.0f;


    // Extract the pose matrix from the current raster.
    m_RasterPose =  m_CurrentRaster->shot.GetWorldToExtrinsicsMatrix().transpose() ;


    // Define the bias matrix that will enable to go from clipping space to texture space.
    const float biasMatData[16] = { 0.5f, 0.0f, 0.0f, 0.0f,
                                    0.0f, 0.5f, 0.0f, 0.0f,
                                    0.0f, 0.0f, 0.5f, 0.0f,
                                    0.5f, 0.5f, 0.5f, 1.0f };
    vcg::Matrix44f biasMat( biasMatData );


    // Update the shadow map projection matrix.
    m_ShadowProj = m_RasterPose * m_RasterProj * biasMat;
}


void DecorateRasterProjPlugin::updateColorTexture()
{
    glPushAttrib( GL_TEXTURE_BIT );

    const int w = m_CurrentRaster->currentPlane->image.width();
    const int h = m_CurrentRaster->currentPlane->image.height();


    // Recover image data and convert pixels to the adequate format for transfer onto the GPU.
    GLubyte *texData = new GLubyte [ 3*w*h ];
    for( int y=h-1, n=0; y>=0; --y )
        for( int x=0; x<w; ++x )
        {
            QRgb pixel = m_CurrentRaster->currentPlane->image.pixel(x,y);
            texData[n++] = (GLubyte) qRed  ( pixel );
            texData[n++] = (GLubyte) qGreen( pixel );
            texData[n++] = (GLubyte) qBlue ( pixel );
        }


    // Create and initialize the OpenGL texture object.
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );
    m_ColorTexture.Create( GL_RGB,
                           w,
                           h,
                           GL_RGB,
                           GL_UNSIGNED_BYTE,
                           texData );
    m_ColorTexture.SetFiltering( GL_LINEAR );


    delete [] texData;
    glPopAttrib();
}


void DecorateRasterProjPlugin::updateDepthTexture()
{
    glPushAttrib( GL_TEXTURE_BIT   |
                  GL_ENABLE_BIT    |
                  GL_POLYGON_BIT   |
                  GL_CURRENT_BIT   |
                  GL_TRANSFORM_BIT |
                  GL_VIEWPORT_BIT  );

    const int w = m_CurrentRaster->currentPlane->image.width();
    const int h = m_CurrentRaster->currentPlane->image.height();


    // Create and initialize the OpenGL texture object used to store the shadow map.
    m_DepthTexture.Create( GL_DEPTH_COMPONENT,
                           w,
                           h,
                           GL_DEPTH_COMPONENT,
                           GL_INT,
                           NULL );

    m_DepthTexture.SetFiltering( GL_LINEAR );
    m_DepthTexture.SetParam( GL_TEXTURE_COMPARE_MODE_ARB, GL_COMPARE_R_TO_TEXTURE );
    m_DepthTexture.SetParam( GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
    m_DepthTexture.SetParam( GL_DEPTH_TEXTURE_MODE_ARB, GL_INTENSITY );


    // Perform an off-screen rendering pass so as to generate the a depth map of the model
    // from the viewpoint of the current raster's camera.
    glMatrixMode( GL_PROJECTION );
    glPushMatrix();
    glLoadMatrixf( (GLfloat*) m_RasterProj.V() );

    glMatrixMode( GL_MODELVIEW );
    glPushMatrix();
    glLoadMatrixf( (GLfloat*) m_RasterPose.V() );

    GPU::FrameBuffer fbuffer( w, h );
    fbuffer.Attach( GL_DEPTH_ATTACHMENT_EXT, m_DepthTexture );
    fbuffer.Bind();

    glEnable( GL_DEPTH_TEST );
    glEnable( GL_POLYGON_OFFSET_FILL );
    glPolygonOffset( 2.0f, 2.0f );

    glClear( GL_DEPTH_BUFFER_BIT );
    for( QMap<int,MeshDrawer>::iterator m=m_Scene.begin(); m!=m_Scene.end(); ++m )
        m->drawShadow();

    fbuffer.Unbind();

    glPopMatrix();
    glMatrixMode( GL_PROJECTION );
    glPopMatrix();

    glPopAttrib();
}


void DecorateRasterProjPlugin::updateCurrentRaster( MeshDocument &m )
{
    // Update the stored raster with the one provided by the mesh document.
    // If both are identical, the update is simply skiped.
    if( m.rm() == m_CurrentRaster )
        return;

    m_CurrentRaster = m.rm();

    updateColorTexture();
    updateShadowProjectionMatrix();
    updateDepthTexture();
}


bool DecorateRasterProjPlugin::initShaders( std::string &logs )
{
    GPU::Shader::VertPg vpg;
    GPU::Shader::FragPg fpg;

    std::string basename = PluginManager::getBaseDirPath().append("/shaders/raster_proj/raster_proj").toStdString();

    return vpg.CompileSrcFile( basename+".vert", &logs ) &&
           fpg.CompileSrcFile( basename+".frag", &logs ) &&
           m_ShadowMapShader.Attach( vpg )
                            .AttachAndLink( fpg, &logs );
}


bool DecorateRasterProjPlugin::startDecorate( QAction          *act,
                                              MeshDocument     & /*m*/,
                                              RichParameterSet * /*par*/,
                                              GLArea           * /*gla*/ )
{
    switch( ID(act) )
    {
        case DP_PROJECT_RASTER:
        {
            glPushAttrib( GL_ALL_ATTRIB_BITS );

            GLenum err = glewInit();
            if( err != GLEW_OK )
            {
                qWarning( (std::string("Impossible to load GLEW library.")+(char*)glewGetErrorString(err)).c_str() );
                return false;
            }
            Log( "GLEW library correctly initialized." );

            std::string logs;
            if( !initShaders(logs) )
            {
                qWarning( ("Error while initializing shaders.\n"+logs).c_str() );
                return false;
            }
            Log( "Shaders correctly loaded." );

            s_AreVBOSupported = glewIsSupported( "GL_ARB_vertex_buffer_object" );

            m_Scene.clear();
            m_CurrentMesh = NULL;
            m_CurrentRaster = NULL;

            glPopAttrib();
            return true;
        }
        default: assert( 0 );
    }

    return false;
}


void DecorateRasterProjPlugin::endDecorate( QAction          *act,
                                            MeshDocument     & /*m*/,
                                            RichParameterSet * /*par*/,
                                            GLArea           * /*gla*/ )
{
    switch( ID(act) )
    {
        case DP_PROJECT_RASTER:
        {
            glPushAttrib( GL_ALL_ATTRIB_BITS );

            m_Scene.clear();
            m_CurrentMesh = NULL;
            m_CurrentRaster = NULL;
            m_ShadowMapShader.Release();
            m_DepthTexture.Release();
            m_ColorTexture.Release();

            glPopAttrib();
            break;
        }
        default: assert( 0 );
    }
}


void DecorateRasterProjPlugin::setPointParameters( MeshDrawer &md,
                                                   RichParameterSet *par )
{
    if( par->getBool("MeshLab::Appearance::pointSmooth") )
        glEnable( GL_POINT_SMOOTH );
    else
        glDisable( GL_POINT_SMOOTH );
    
    glPointSize( par->getFloat("MeshLab::Appearance::pointSize") );

    if( glPointParameterfv )
    {
        if( par->getBool("MeshLab::Appearance::pointDistanceAttenuation") )
        {
            vcg::Matrix44f mvMat;
            glGetFloatv( GL_MODELVIEW_MATRIX, mvMat.V() );
            vcg::Transpose( mvMat );
            float camDist = vcg::Norm( mvMat * md.mm()->cm.Tr * md.mm()->cm.bbox.Center() );

            float quadratic[3] = { 0.0f, 0.0f, 1.0f/(camDist*camDist) };
            glPointParameterfv( GL_POINT_DISTANCE_ATTENUATION, quadratic );
            glPointParameterf( GL_POINT_SIZE_MAX, 16.0f );
            glPointParameterf( GL_POINT_SIZE_MIN, 1.0f );
        }
        else
        {
            float quadratic[3] = { 1.0f, 0.0f, 0.0f };
            glPointParameterfv( GL_POINT_DISTANCE_ATTENUATION, quadratic );
        }
    }
}


void DecorateRasterProjPlugin::decorate( QAction           *act,
                                         MeshDocument      &m  ,
                                         RichParameterSet  *par,
                                         GLArea            *gla,
                                         QPainter          * /*p*/)
{
    switch( ID(act) )
    {
        case DP_PROJECT_RASTER:
        {
            glPushAttrib( GL_ALL_ATTRIB_BITS );

            updateCurrentMesh( m, *par );
            updateCurrentRaster( m );

            glEnable( GL_DEPTH_TEST );

            RenderMode rm = gla->getCurrentRenderMode();
            bool notDrawn = false;
            switch( rm.drawMode )
            {
                case vcg::GLW::DMPoints:
                {
                    glPolygonMode( GL_FRONT_AND_BACK, GL_POINT );
                    glEnable( GL_POLYGON_OFFSET_POINT );
                    break;
                }
                case vcg::GLW::DMHidden:
                case vcg::GLW::DMWire:
                {
                    glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
                    glEnable( GL_POLYGON_OFFSET_LINE );
                    break;
                }
                case vcg::GLW::DMFlat:
                case vcg::GLW::DMFlatWire:
                case vcg::GLW::DMSmooth:
                {
                    glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
                    glEnable( GL_POLYGON_OFFSET_FILL );
                    break;
                }
                default: notDrawn = true;
            }

            if( !notDrawn )
            {
                glEnable( GL_BLEND );
                glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                glPolygonOffset( -2.0f, 1.0f );
                glEnable( GL_COLOR_MATERIAL );
                glColor3ub( 255, 255, 255 );

                glEnable( GL_PROGRAM_POINT_SIZE );
                m_ShadowMapShader.Bind();
                m_ColorTexture.Bind( 0 );
                m_DepthTexture.Bind( 1 );
                m_ShadowMapShader.SetSampler( "u_ColorMap" , 0 );
                m_ShadowMapShader.SetSampler( "u_DepthMap" , 1 );
                m_ShadowMapShader.SetUniform( "u_ProjMat"  , m_ShadowProj.V() );
                m_ShadowMapShader.SetUniform( "u_Viewpoint", m_CurrentRaster->shot.GetViewPoint().V() );
                vcg::Matrix44f lightToObj = ( gla->trackball.InverseMatrix() * gla->trackball_light.Matrix() ).transpose();
                m_ShadowMapShader.SetUniform( "u_LightToObj", lightToObj.V() );
                GLint islightActivated = rm.lighting && par->getBool("MeshLab::Decoration::ProjRasterLighting");
                m_ShadowMapShader.SetUniform( "u_IsLightActivated", &islightActivated );
                float alpha = par->getFloat( "MeshLab::Decoration::ProjRasterAlpha" );
                m_ShadowMapShader.SetUniform( "u_AlphaValue", &alpha );

                for( QMap<int,MeshDrawer>::iterator m=m_Scene.begin(); m!=m_Scene.end(); ++m )
                {
                    if( rm.drawMode == vcg::GLW::DMPoints )
                        setPointParameters( m.value(), par );
                    m_ShadowMapShader.SetUniform( "u_ModelXf", vcg::Matrix44f(m->mm()->cm.Tr).transpose().V() );
                    m->draw();
                }

                m_ShadowMapShader.Unbind();
                m_DepthTexture.Unbind();
                m_ColorTexture.Unbind();
            }

            glPopAttrib();
            break;
        }
        default: assert(0);
    }
}




Q_EXPORT_PLUGIN(DecorateRasterProjPlugin)
