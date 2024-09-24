
#include "editor_application/editor_application.hpp"

#include "engine/scripting/scripting_engine.hpp"
#include <sol/sol.hpp>

namespace bubble
{
constexpr WindowSize WINDOW_SIZE{ 1200, 720 };
constexpr uvec2 VIEWPORT_SIZE{ 800, 640 };

BubbleEditor::BubbleEditor()
    : mEditorMode( EditorMode::Editing ),
      mWindow( Window( "Bubble", WINDOW_SIZE ) ),
      mSceneViewport( Framebuffer( Texture2DSpecification::CreateRGBA8( VIEWPORT_SIZE ),
                                   Texture2DSpecification::CreateDepth( VIEWPORT_SIZE ) ) ),
      mObjectIdViewport( Framebuffer( Texture2DSpecification::CreateObjectId( VIEWPORT_SIZE ),
                                      Texture2DSpecification::CreateDepth( VIEWPORT_SIZE ) ) ),
      mSceneCamera( SceneCamera( mWindow.GetWindowInput() ) ),
      mScriptingEngine( mWindow.GetWindowInput(), mProject.mLoader, mProject.mScene ),
      mResourcesHotReloader( mProject.mLoader ),
      mEditorUserInterface( *this ),
      mObjectIdShader( LoadShader( OBJECT_PICKING_SHADER ) )
{}

void BubbleEditor::Run()
{
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_BEGIN
#else
    while ( !mWindow.ShouldClose() )
#endif
    {
        // Poll events
        mWindow.PollEvents();
        mTimer.OnUpdate();
        auto deltaTime = mTimer.GetDeltaTime();

        // Draw scene
        switch ( mEditorMode )
		{
			case EditorMode::Editing:
			{
				mSceneCamera.OnUpdate( deltaTime );
				mResourcesHotReloader.OnUpdate();
				mEditorUserInterface.OnUpdate( deltaTime );

                //UpdateScripts();
                auto view = mProject.mScene.GetRuntimeView( { "ScriptComponent" } );
                for ( auto entity : view )
                    mScriptingEngine.OnUpdate( entity.GetComponent<ScriptComponent>().mScript );


				DrawProjectScene();
                // ImGui
                Framebuffer::BindWindow( mWindow );
                mWindow.ImGuiBegin();
                mEditorUserInterface.OnDraw( deltaTime );
                mWindow.ImGuiEnd();
				break;
			}
			case EditorMode::Runing:
				break;
        }
        mWindow.OnUpdate();
    }
#ifdef __EMSCRIPTEN__
    EMSCRIPTEN_MAINLOOP_END;
#endif
}

void BubbleEditor::OpenProject( const path& projectPath )
{
    LogInfo( "Open project {}", projectPath.string() );
    mUINeedUpdateProjectWindow = true;
    mProject.Open( projectPath );
}

void BubbleEditor::UpdateScripts()
{
    mProject.mScene.ForEach<ScriptComponent>( 
    [&]( Entity, ScriptComponent& script )
    {
        mScriptingEngine.OnUpdate( script.mScript );
    } );
}

void BubbleEditor::DrawProjectScene()
{
    mRenderer.SetUniformBuffers( mSceneCamera, mSceneViewport );

    // Draw scene
    mSceneViewport.Bind();
    mRenderer.ClearScreen( vec4( 0.2f, 0.3f, 0.3f, 1.0f ) );
    mProject.mScene.ForEach<ModelComponent, ShaderComponent, TransformComponent>(
    [&]( Entity entity,
         ModelComponent& model,
         ShaderComponent& shader,
         TransformComponent& transform )
    {
        mRenderer.DrawModel( model, transform.Transform(), shader );
    } );

    // Draw scene's objectId
    mObjectIdViewport.Bind();
    mRenderer.ClearScreenUint( uvec4( 0 ) );
    mProject.mScene.ForEach<ModelComponent, TransformComponent>(
    [&]( Entity entity,
         ModelComponent& model,
         TransformComponent& transform )
    {
        mObjectIdShader->SetUni1ui( "uObjectId", (u32)entity );
        mRenderer.DrawModel( model, transform.Transform(), mObjectIdShader );
    } );
}

}
