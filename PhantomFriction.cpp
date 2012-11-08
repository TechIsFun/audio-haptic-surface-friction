#include "PhantomFriction.h"

/**
 * Struct representing a shape in the scene that can be felt, 
 * touched, transformed.
 */
struct DraggableObject
{
    HLuint shapeId;
	GLuint displayList;
    hduMatrix transform;
    float hap_stiffness;
    float hap_damping;
    float hap_static_friction;
    float hap_dynamic_friction;
};

//List of all draggable objects in scene.
std::vector<DraggableObject> draggableObjects;

//Object currently being dragged (index into draggableObjects). 
int gCurrentDragObj = -1;

//Position and orientation of proxy at start of drag. 
hduVector3Dd gStartDragProxyPos;
hduQuaternion gStartDragProxyRot;

//Position and orientation of drag object at start of drag. 
hduMatrix gStartDragObjTransform;

//Flag for enabling/disabling axis snap on drag.
bool gAxisSnap = true;

//Flag for enabling/disabling rotation.
bool gRotate = true;

/**
 * Main function.
 */
int main(int argc, char *argv[])
{
	// Init shared memory.
	InitMemory();

	// Ask for experiment's subject's name and age.
	cout<<"Inserisci il tuo nome (cognome_nome): ";
	cin>>nome;
	cout<<"Inserisci l'eta': ";
	cin>>eta;
	cout<<"Inizializzazione esperimento..."<<endl;

	hlEventd(HL_EVENT_MOTION_ANGULAR_TOLERANCE, kPI);

	// Init graphic interface.
	glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(windowWidth, windowHeight);
    main_window = glutCreateWindow("Phantom Friction");
    //Set glut callback functions.
    glutDisplayFunc(glutDisplay);
	glutMotionFunc(glutMotion);

    GLUI_Master.set_glutReshapeFunc(glutReshape);  
    GLUI_Master.set_glutKeyboardFunc(glutKeyboard);
    GLUI_Master.set_glutMouseFunc(glutMouse);
    GLUI_Master.set_glutSpecialFunc(NULL);
    
    // The GLUT main loop won't return control, so we need to perform cleanup
    // using an exit handler.
    atexit(exitHandler);
    
	// Init the experiment.
    initScene();
	defineScenario();
	InitOutputFile();
    
    // Register the idle callback with GLUI.
    GLUI_Master.set_glutIdleFunc(glutIdle);

	#if (defined(_SHADOWMAPPING))
	// Init the shadow mapping.
	ShadowMappingInit();
	RegenerateShadowMap(light0_position, shadowSize);
	#endif

    glutMainLoop();
}

/**
 * GLUI control callback.
 */
void control_cb(int control)
{
	if( control == OBJECT_TYPE_ID )
    {
        DraggableObject& obj = draggableObjects[obj_type]; 
        hap_stiffness = obj.hap_stiffness;
        hap_damping = obj.hap_damping;
        hap_static_friction = obj.hap_static_friction;
        hap_dynamic_friction = obj.hap_dynamic_friction;
    }
    else if ( control == VIEW_ROTATE_ID ) { updateCamera(); }
    else if ( control == VIEW_PAN_ID ) { updateCamera(); }
    else if ( control == VIEW_ZOOM_ID ) { updateCamera(); }
}

/**
 * Initializes GLUI user interface.
 */
void initGLUI()
{     
    printf("GLUI version: %3.2f\n", GLUI_Master.get_version());
    
	glui= GLUI_Master.create_glui_subwindow(main_window, GLUI_SUBWINDOW_RIGHT);
	glui->add_statictext("1. Come hai percepito");
	glui->add_statictext("   la superficie?");
	expPanel = glui->add_panel("", GLUI_PANEL_EMBOSSED);
	expRad= glui->add_radiogroup_to_panel(expPanel);
	glui->add_radiobutton_to_group(expRad,"Molto liscia");
	glui->add_radiobutton_to_group(expRad,"Liscia");
	glui->add_radiobutton_to_group(expRad,"Poco liscia");
	glui->add_radiobutton_to_group(expRad,"Poco ruvida");
	glui->add_radiobutton_to_group(expRad,"Ruvida");
	glui->add_radiobutton_to_group(expRad,"Molto ruvida");

	glui->add_statictext("2. Con quale sicurezza");
	glui->add_statictext("   hai dato la risposta?");
	expPanel2 = glui->add_panel("", GLUI_PANEL_EMBOSSED);
	//expPanel2 = glui->add_panel("hai dato la risposta?", GLUI_PANEL_EMBOSSED);
	expRad2 = glui->add_radiogroup_to_panel(expPanel2);
	glui->add_radiobutton_to_group(expRad2,"Molto sicuro");
	glui->add_radiobutton_to_group(expRad2,"Sicuro");
	glui->add_radiobutton_to_group(expRad2,"Poco sicuro");

	glui->add_statictext("3. Procedi con il");
	glui->add_statictext("   prossimo scenario");
	expPanel3 = glui->add_panel("", GLUI_PANEL_EMBOSSED);
	glui->add_button_to_panel(expPanel3,"Avanti >>",-1,nextButton);
	
	glui->set_main_gfx_window(main_window);
}

/**
 * GLUT callback for redrawing the view.  Use this to perform graphics rate 
 * processing.
 */
void glutDisplay()
{    
	// Redraw with shadows
	#if(defined(_SHADOWMAPPING))
	ShadowMappingFirst(gCameraPosWC);
	drawScene();
	ShadowMappingSecond();
	RegenerateShadowMap(light0_position, shadowSize);
	#endif	

	// Redraw without shadows
	#if(!defined(_SHADOWMAPPING))
	drawScene();
	drawPrompts();
	glutSwapBuffers();
	#endif
}

/**
 * GLUT callback for reshaping the window.  This is the main place where the 
 * viewing and workspace transforms get initialized.
 * @param width window width
 * @param height window height
 */
void glutReshape(int width, int height)
{
    gWindowWidth = width;
    gWindowHeight = height;

    int tx, ty;
    GLUI_Master.get_viewport_area(&tx, &ty, &gViewportWidth, &gViewportHeight);
    glViewport(tx, ty, gViewportWidth, gViewportHeight);

    // Compute and set the viewing parameters.
	aspect = (double) gViewportWidth / gViewportHeight;
   
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
	gluPerspective(40, aspect, 274.748, 1898.99);

    // Place the camera.
    gCameraPosWC[0] = 0;
    gCameraPosWC[1] = 140;
	gCameraPosWC[2] = 2*(274.748 + kCanonicalSphereRadius);

    updateCamera();
}

/**
 * GLUT callback for idle state.  Use this to request a redraw.
 */
void glutIdle()
{
    // According to the GLUT specification, the current window is 
    // undefined during an idle callback.  So we need to explicitly change
    // it if necessary.
    if (glutGetWindow() != main_window) 
    {
        glutSetWindow(main_window);
    }
    
    glutPostRedisplay();
    glui->sync_live();

    // Update the selected object's material.
    DraggableObject& obj = draggableObjects[obj_type]; 

    obj.hap_stiffness = hap_stiffness;
    obj.hap_damping = hap_damping;
    obj.hap_static_friction = hap_static_friction;
    obj.hap_dynamic_friction = hap_dynamic_friction;
}

/**
 * Initializes the scene. Handles initializing both OpenGL and HDAPI. 
 */
void initScene()
{
    initGLUI();
	initGL();
	initHL();
	createDraggableObjects(false,0);
}

/**
 * Cleans up.
 */
void exitHandler()
{
	#if (defined(_HAPTIC_))
    // Shutdown the haptic mouse.
    hmShutdownMouse();

    // Free up the haptic rendering context.
    hlMakeCurrent(NULL);
    if (ghHLRC != NULL)
    {
        hlDeleteContext(ghHLRC);
    }

    // Free up the haptic device.
    if (ghHD != HD_INVALID_HANDLE)
    {
        hdDisableDevice(ghHD);
    }
	#endif

	// Close the shared memory area.
	int errorClose = ClosePhantomMemory();
}

/**
 * Sets up general OpenGL rendering properties: lights, depth buffering, etc. 
 */
void initGL()
{
	#if(defined(_STENCIL))
	// Any three points on the ground (counter clockwise order)
    GLTVector3 points[3] = {{ -30.0f, -149.0f, -20.0f },
                            { -30.0f, -149.0f, 20.0f },
                            { 40.0f, -149.0f, 20.0f }};
	#endif

	// Set background color
    glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
	
    // Turn on antialiasing, and give hint to do the best
	// job possible.
	#if(defined(_STENCIL))
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glEnable(GL_POINT_SMOOTH);
    glHint(GL_POINT_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glEnable(GL_POLYGON_SMOOTH);
    glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	#endif
	
	// Enable depth buffering for hidden surface removal.
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_DEPTH_TEST);
	
	#if(defined(_STANCIL))
	// Counter clock-wise polygons face out
    glFrontFace(GL_CCW);		
	#endif

    // Cull back faces.
    glCullFace(GL_BACK);
    glEnable(GL_CULL_FACE);
    
	#if(defined(_STENCIL))
    // Set lighting parameters.
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight0);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight0);
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0,GL_SPECULAR,specular);	
	glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
	
    // All materials hereafter have full specular reflectivity
    // with a high shine
    glMaterialfv(GL_FRONT, GL_SPECULAR,specref);
    glMateriali(GL_FRONT,GL_SHININESS,128);

	// Calculate projection matrix to draw shadow on the ground
    gltMakeShadowMatrix(points, lightPos, shadowMat);
	#endif

	#if(defined(_TEXTURE)) 
	loadTexture();	
	#endif
}

/**
 * Sets up/initializes haptic rendering library.
 */
void initHL()
{
	#if (defined(_HAPTIC_))
	HDErrorInfo error;
    ghHD = hdInitDevice(HD_DEFAULT_DEVICE);
	
    if (HD_DEVICE_ERROR(error = hdGetError()))
    {
        hduPrintError(stderr, &error, "Failed to initialize haptic device");
        fprintf(stderr, "Press any key to exit");
        getchar();
		int errorClose = ClosePhantomMemory();
        exit(1);
    }
    
    // Create a haptic context for the device.  The haptic context maintains 
    // the state that persists between frame intervals and is used for
    // haptic rendering.
    ghHLRC = hlCreateContext(ghHD);
    hlMakeCurrent(ghHLRC);
	#endif

    // Generate a shape id to hold the axis snap constraint.
    gAxisId = hlGenShapes(1);

	// Callback for touching event
	hlAddEventCallback(HL_EVENT_TOUCH, HL_OBJECT_ANY, HL_COLLISION_THREAD,
		&touchCallback, NULL);

	// Callback for untouching event
	hlAddEventCallback(HL_EVENT_UNTOUCH, HL_OBJECT_ANY, HL_COLLISION_THREAD,
		&untouchCallback, NULL);

	// Callback for motion+touching event
	hdScheduleAsynchronous(velocitaCallback,0,HD_MIN_SCHEDULER_PRIORITY);

	hlEnable(HL_HAPTIC_CAMERA_VIEW);

	hlHinti(HL_SHAPE_FEEDBACK_BUFFER_VERTICES, 100);

	// Initialize the haptic mouse.
    //hmInitializeMouse(ghHLRC, "GLUT", "Phantom Friction");

    // Save off the initial workspace, since we will be modifying it for the
    // haptic mouse to allow for motion outside of the viewport.
    hlGetDoublev(HL_WORKSPACE, gInitWorkspace);
}

/**
 * Creates the objects that can be seen, felt and dragged around.
 * @param texturized true if the objects have texture.
 * @param nShadow define objects' color.
 */
void createDraggableObjects(const GLboolean& texturized, const GLint& nShadow)
{
	// Create a bunch of shapes and add them to the draggable object vector.
    DraggableObject dro;

	// Set-up some haptic properties
	dro.hap_stiffness = 1.0;
    dro.hap_damping = 0.0;
    dro.hap_static_friction = 0.4;
    dro.hap_dynamic_friction = 0.4;

	dro.shapeId = hlGenShapes(1);
	
	// Draw a surface.
	dro.displayList = glGenLists(1);
	dro.transform = hduMatrix::createTranslation(surfx,surfy,surfz);
    glNewList(dro.displayList, GL_COMPILE);
	drawSurface(1000.0f, 800.0f, 20.0f, texturized);
	glEndList();
    draggableObjects.push_back(dro);
	
	#if (defined(_HAPTIC_))
	// Uncomment the following if you want to drag objects.
    // Add event callbacks for button down on each of the shapes.
    // Callbacks will set that shape to be the drag object.
	/**
    for (int i = 0; i < draggableObjects.size(); ++i)
    {
        // Pass the index of the object as userdata.
        hlAddEventCallback(HL_EVENT_1BUTTONDOWN, 
                           draggableObjects[i].shapeId, 
                           HL_CLIENT_THREAD, 
                           buttonDownClientThreadCallback, 
                           reinterpret_cast<void *>(i));
    }
	    // Add an event callback on button to clear the drag object
    // and end dragging.
    hlAddEventCallback(HL_EVENT_1BUTTONUP, HL_OBJECT_ANY, HL_CLIENT_THREAD, 
                       buttonUpClientThreadCallback, NULL);
					   **/
	#endif
}

/**
 * Draws the objects that can be seen and felt. 
 * @param texturized true if the objects have texture.
 * @param nShadow define objects' color.
 */
void drawDraggableObjects(const GLboolean& texturized, const GLint& nShadow)
{
    hlTouchModel(HL_CONTACT);
    hlTouchableFace(HL_FRONT);

    for (int i = 0; i < draggableObjects.size(); ++i)
    {
        const DraggableObject& obj = draggableObjects[i];
        glPushMatrix(); // Position and orient the object.
        glMultMatrixd(obj.transform);
		// Draw the object graphically.
        glCallList(obj.displayList);

		#if(defined(_HAPTIC_))
        // Draw the object haptically (but not if it is being dragged).
        if (i != gCurrentDragObj && !hmIsMouseActive())
        {
            hlBeginShape(HL_SHAPE_FEEDBACK_BUFFER, obj.shapeId);

            hlMaterialf(HL_FRONT_AND_BACK, HL_STIFFNESS, obj.hap_stiffness);
            hlMaterialf(HL_FRONT, HL_DAMPING, obj.hap_damping);
            hlMaterialf(HL_FRONT, HL_STATIC_FRICTION, obj.hap_static_friction);
            hlMaterialf(HL_FRONT, HL_DYNAMIC_FRICTION, obj.hap_dynamic_friction);

            glCallList(obj.displayList);

            hlEndShape();
        }
		#endif

        glPopMatrix();
    }
}

/**
 * Sets the modelview transform from scratch.  Applies the current view 
 * orientation and scale. 
 */
void updateCamera()
{	
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();    

    gluLookAt(gCameraPosWC[0], gCameraPosWC[1], gCameraPosWC[2], 0, 0, 0, 0, 1, 0);
    
    glTranslatef(gCameraTranslationX, gCameraTranslationY, 0);
    glMultMatrixd(gCameraRotation);
    glTranslatef(obj_pos[0], obj_pos[1], obj_pos[2]);
    glMultMatrixf(view_rotate);
    glScaled(gCameraScale, gCameraScale, gCameraScale);

	#if (defined(_HAPTIC_))
    updateHapticMapping();
	#endif

    glutPostRedisplay();
}

/**
 * Uses the current OpenGL viewing transforms to initialize a transform for the
 * haptic device workspace so that it's properly mapped to world coordinates. 
 */
void updateHapticMapping(void)
{
	GLdouble modelview[16];
    GLdouble projection[16];
    GLint viewport[4];

    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);
    glGetIntegerv(GL_VIEWPORT, viewport);
    
    // Modify the workspace dimensions mapped to the view so that the user
    // can move outside of the viewport and access the GLUI interface.
    double t = (double) gViewportWidth / gWindowWidth;
	
	double xMaxProportion = hduLerp(gInitWorkspace[0], gInitWorkspace[3], t);

    hlWorkspace(gInitWorkspace[0], gInitWorkspace[1], gInitWorkspace[2],
                xMaxProportion, gInitWorkspace[4], gInitWorkspace[5]);

    // Fit haptic workspace to view volume.
    hlMatrixMode(HL_TOUCHWORKSPACE);
    hlLoadIdentity();
    hluFitWorkspace(projection);    

    // Compute cursor scale.
    gCursorScale = hluScreenToModelScale(modelview, projection, viewport);
    gCursorScale *= CURSOR_SIZE_PIXELS;

    // Provide the current viewing transforms to HapticMouse so it can
    // map the device to the screen.
    hmSetMouseTransforms(modelview, projection, viewport);
}

/**
 * Displays a cursor using the current haptic device proxy transform and the
 * mapping between the workspace and world coordinates
 * @param h true for haptic rendering.
 * @param nShadow define cursor's color.
 */
void redrawCursor(const boolean& h, const int& nShadow)
{    
    GLUquadricObj *qobj = 0;
    glPushAttrib(GL_CURRENT_BIT | GL_ENABLE_BIT | GL_LIGHTING_BIT);
    glPushMatrix();
    if (!gCursorDisplayList)
    {
        gCursorDisplayList = glGenLists(1);
        glNewList(gCursorDisplayList, GL_COMPILE);
        qobj = gluNewQuadric();
        
		gluQuadricDrawStyle(qobj,GLU_FILL);
		gluQuadricNormals(qobj,GLU_SMOOTH);
		gluQuadricOrientation(qobj,GLU_OUTSIDE);
		gluCylinder(qobj, 0, 5, 15, 30, 1);
		glTranslatef(0, 0, 15);
		gluDisk(qobj, 0, 5, 30, 1);
		gluDeleteQuadric(qobj);

        glEndList();
    }  
    
    // Apply the local position/rotation transform of the haptic device proxy.
	if(h)
	{
		hlGetDoublev(HL_PROXY_TRANSFORM, proxytransform);
		glMultMatrixd(proxytransform);
	}
    
    // Apply the local cursor scale factor.
    //glScaled(gCursorScale, gCursorScale, gCursorScale);
    
	glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
	if(nShadow == 0) { glColor4f(.2f, .2f, .7f, 1.0f); }
    else glColor4f(.4f, .4f, .4f, .4f);
    
    glCallList(gCursorDisplayList);
    glPopMatrix(); 
    glPopAttrib();
}


/**
 * The main routine for displaying the scene.
 */
void drawScene()
{    
	#if (defined(_HAPTIC_))
    hlBeginFrame();
	#endif

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    #if (defined(_HAPTIC_))
	// Any client thread button callbacks get triggered here.
    hlCheckEvents();

    // Draw any haptic mouse scene objects.
    hmRenderMouseScene();

    // Check if button on stylus is down - if so draw the coordinate axes and 
    // move the drag object.
    HLboolean buttDown;
    hlGetBooleanv(HL_BUTTON1_STATE, &buttDown);
	#endif
	
	#if (defined(_HAPTIC_))
    if (buttDown && !hmIsMouseActive())
    {
        if (gAxisSnap)
        {
            // Graphically render the axes.
            drawAxes(gAxisCenter);
						
            // Make sure proxy resolution is on.  The event handler
            // turns it off but it must be on for shapes to be felt.
            hlEnable(HL_PROXY_RESOLUTION);

            // Haptically render the coordinate axes as a feedback buffer 
            // shape.
            hlBeginShape(HL_SHAPE_FEEDBACK_BUFFER, gAxisId);

            // Make it a constraint to the cursor will stick to the axes.
            hlTouchModel(HL_CONSTRAINT);

            // Snap distance allows user to pull off of the constraint
            // if the user moves beyond that snap distance.
            hlTouchModelf(HL_SNAP_DISTANCE, 1.5);

            // Call the OpenGL commands to draw the axes, but this time
            // they will be used for haptics.
            drawAxes(gAxisCenter);

            hlEndShape();
        }
		
        // "Drag" the current drag object, if one is current.
        if (gCurrentDragObj != -1)
        {
            updateDragObjectTransform();
        }
    }
	#endif
	//glEnable(GL_LIGHTING);
    //glLightfv(GL_LIGHT0,GL_POSITION,light0_position);
	
	// Draw objects
	drawDraggableObjects(false,0);
	
	#if(!defined(_STENCIL))
	glTranslatef(traslx, trasly, traslz);
	glRotatef(xRot,1,0,0);
	drawCone(false,0);
	#endif
	
	#if(defined(_STENCIL))
	glPushMatrix();
	glEnable(GL_LIGHTING);
    glLightfv(GL_LIGHT0,GL_POSITION,lightPos);
	
	//Draw cursor
	if (!hmIsMouseActive())
    {
		redrawCursor(true,0);
	}

	glPopMatrix();
	glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);
    glPushMatrix();

	// Draw cursor shadow
	hlGetDoublev(HL_PROXY_POSITION, posHD);
	if(posHD[1] > minCursorY )
	{
		glMultMatrixf((GLfloat *)shadowMat);
		redrawCursor(true,1);
	}

	// Restore the projection to normal
	glPopMatrix();

	// Draw the light source
	/**
    glPushMatrix();
    glTranslatef(lightPos[0],lightPos[1], lightPos[2]);
    glColor3ub(255,255,0);
    glutSolidSphere(5.0f,10,10);
    glPopMatrix();
	**/
	#endif
    // Restore lighting state variables
	glEnable(GL_DEPTH_TEST);

	#if (defined(_HAPTIC_))
    if (!hmIsMouseActive())
    {
        //redrawCursor(true);
    }
	#endif

    #if (defined(_HAPTIC_))
	hlEndFrame();
	#endif
}

/**
 * GLUT callback for responding to mouse button presses. Detecst whether to
 * initiate a point snapping, view rotation or view scale.
 */
void glutMouse(int button, int state, int x, int y)
{
	//drawPrompts();

    if (state == GLUT_DOWN)
    {
        //if (button == GLUT_LEFT_BUTTON) { gIsRotatingCamera = true; }
        //else if (button == GLUT_RIGHT_BUTTON) { gIsScalingCamera = true; }
        //else if (button == GLUT_MIDDLE_BUTTON) { gIsTranslatingCamera = true; }
        gLastMouseX = x;
        gLastMouseY = y;
    }
    else
    {
        gIsRotatingCamera = false;
        gIsScalingCamera = false;
        gIsTranslatingCamera = false;
    }
}

/**
 * This routine is used by the view rotation code for simulating a virtual
 * trackball.  This math computes the z height for a 2D projection onto the
 * surface of a 2.5D sphere.  When the input point is near the center of the
 * sphere, this routine computes the actual sphere intersection in Z.  When 
 * the input point moves towards the outside of the sphere, this routine will 
 * solve for a hyperbolic projection, so that it still yields a meaningful answer.
 **/
double projectToTrackball(double radius, double x, double y)
{
    static const double kUnitSphereRadius2D = sqrt(2.0);
    double z;

    double dist = sqrt(x * x + y * y);
    if (dist < radius * kUnitSphereRadius2D / 2.0)
    {
        // Solve for sphere case.
        z = sqrt(radius * radius - dist * dist);
    }
    else
    {
        // Solve for hyperbolic sheet case.
        double t = radius / kUnitSphereRadius2D;
        z = t * t / dist;
    }

    return z;
}

/**
 * GLUT callback for mouse motion, which is used for controlling the view
 * rotation and scaling.
 */
void glutMotion(int x, int y)
{
    if (gIsRotatingCamera)
    {
        static const double kTrackBallRadius = 0.8;   

        hduVector3Dd lastPos;
		hduVector3Dd currPos;
		hduVector3Dd rotateVec;
        lastPos[0] = gLastMouseX * 2.0 / gViewportWidth - 1.0;
        lastPos[1] = (gViewportHeight - gLastMouseY) * 2.0 / gViewportHeight - 1.0;
        lastPos[2] = projectToTrackball(kTrackBallRadius, lastPos[0], lastPos[1]);
        currPos[0] = x * 2.0 / gViewportWidth - 1.0;
        currPos[1] = (gViewportHeight - y) * 2.0 / gViewportHeight - 1.0;
        currPos[2] = projectToTrackball(kTrackBallRadius, currPos[0], currPos[1]);

        currPos.normalize();
        lastPos.normalize();

		
        rotateVec = lastPos.crossProduct(currPos);
        
        double rotateAngle = asin(rotateVec.magnitude());
        if (!hduIsEqual(rotateAngle, 0.0, DBL_EPSILON))
        {
            hduMatrix deltaRotation = hduMatrix::createRotation(
                rotateVec, rotateAngle);            
            gCameraRotation.multRight(deltaRotation);
            updateCamera();
        }
    }
    if (gIsTranslatingCamera)
    {
        gCameraTranslationX += 10 * double(x - gLastMouseX)/gViewportWidth;
        gCameraTranslationY -= 10 * double(y - gLastMouseY)/gViewportHeight;

        updateCamera();
    }
    else if (gIsScalingCamera)
    {
        float y1 = gViewportHeight - gLastMouseY;
        float y2 = gViewportHeight - y;

        gCameraScale *= 1 + (y1 - y2) / gViewportHeight;  

        updateCamera();
    }
    gLastMouseX = x;
    gLastMouseY = y;
}

/**
 * GLUT callback for key presses.
 */
void glutKeyboard(unsigned char key, int x, int y)
{
	/**
    switch (key) 
	{
//    case 'r':
    //case 'R':
        //gRotate = !gRotate;
        //break;
    case 'a':
		pdSurface = ReadSurface();
		printf("surface=%f\n", pdSurface);
		break;
    case 'A':
        break;
	case 'n':
		xRot -= 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize ); #endif
		glutPostRedisplay(); break;
	case 'N':
		xRot += 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize ); #endif
		glutPostRedisplay(); break;
	case 'e':
		traslx -= 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize ); #endif
		glutPostRedisplay(); break;
	case 'E':
		traslx += 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize ); #endif
		glutPostRedisplay(); break;
	case 'r':
		trasly -= 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize ); #endif
		glutPostRedisplay(); cout<<"trasly = "<<trasly<<endl; break;
	case 'R':
		trasly += 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		glutPostRedisplay(); cout<<"trasly = "<<trasly<<endl; break;
	case 't':
		traslz -= 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		glutPostRedisplay(); break;
	case 'T':
		traslz += 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		glutPostRedisplay(); break;
		/**
	case 'e':
		traslx -= 5.0f;
		light0_position[0] += 5.0f;
		updateCamera();
		break;
	case 'E':
		traslx += 5.0f;
		light0_position[0] -= 5.0f;
		//if ( shadowMapOn) RegenerateShadowMap( light0_position, shadowSize );
		updateCamera();
		break;
	case 'r':
		trasly -= 5.0f;
		light0_position[1] -= 5.0f;
		updateCamera();
		break;
	case 'R':
		trasly += 5.0f;
		light0_position[1] += 5.0f;
		//if ( shadowMapOn) RegenerateShadowMap( light0_position, shadowSize );
		updateCamera();
		break;
	case 't':
		traslz -= 5.0f;
		light0_position[2] -= 5.0f;
		updateCamera();
		break;
	case 'T':
		traslz += 5.0f;
		light0_position[2] += 5.0f;
		//if ( shadowMapOn) RegenerateShadowMap( light0_position, shadowSize );
		updateCamera();
		break;
	case 'f':
		factor += 0.5f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		break;
	case 'x':
		light0_position[0] -= 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		//cout<<"light[0]="<<light0_position[0]<<endl;
		//cout<<"cam[0]="<<gCameraPosWC[0]<<endl;
		break;
	case 'X':
		light0_position[0] += 5.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		//cout<<"light[0]="<<light0_position[0]<<endl;
		//cout<<"cam[0]="<<gCameraPosWC[0]<<endl;
		break;
	case 'y':
		light0_position[1] -= 1.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		//cout<<"light[1]="<<light0_position[1]<<endl;
		//cout<<"cam[1]="<<gCameraPosWC[1]<<endl;
		break;		
	case 'Y':
		light0_position[1] += 1.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		//cout<<"light[1]="<<light0_position[1]<<endl;
		//cout<<"cam[1]="<<gCameraPosWC[1]<<endl;
		break;		
	case 'Z':
		light0_position[2] += 1.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		//cout<<"light[2]="<<light0_position[2]<<endl;
		//cout<<"cam[2]="<<gCameraPosWC[2]<<endl;
		break;		
	case 'z':
		light0_position[2] -= 1.0f;
		#if(defined(_SHADOWMAPPING)) 
		RegenerateShadowMap( light0_position, shadowSize );	#endif
		updateCamera();
		//cout<<"light[2]="<<light0_position[2]<<endl;
		//cout<<"cam[2]="<<gCameraPosWC[2]<<endl;
		break;		
		
    }
	**/
}

/**
 * HL callback triggered when surface touch occour.
 **/
void HLCALLBACK touchCallback(HLenum event, HLuint object, HLenum thread,HLcache *cache, void *userdata)
{
	touching = true;
}

/**
 * HL callback triggered when surface untouch occour.
 **/
void HLCALLBACK untouchCallback(HLenum event, HLuint object, HLenum thread,HLcache *cache, void *userdata)
{
	touching = false;
}

/**
 * HD callback at servo loop rate.
 **/
HDCallbackCode HDCALLBACK velocitaCallback(void *pUserData)
{
	hdGetDoublev(HD_CURRENT_VELOCITY, veloHD);
		
	if ( touching )
	{
		hdGetDoublev(HD_CURRENT_FORCE, forceHD);
	
		velocita = sqrt( pow(veloHD[0],2) + pow(veloHD[2],2) );
					
		// memory writing of haptic scenario's data
		// write:	haptic scene data
		//			modulus of  x and y velocities
		//			radix of velocity*normal force (van den doel) (solo velocita normale?)
		setOpeData( &data,
				(float)( sqrt( abs(velocita * forceHD[1]) )/1000 ), 
				velocita/1000,
				0,	//kappa
				sceneDepth[indexScene],  //depth
				0,
				0,
				sceneNoiseFile[indexScene]);
		//cout<<"nForce"<<abs(veloHD[1] * forceHD[1])<<endl;
		WriteOpeData(&data);
		//pdSurface = ;
		// i can use "/b" instead of "/b-a" if a=0
		hap_dynamic_friction = 0.4;
		hap_static_friction = ( ReadSurface() - a )/ ( b );
	}
	else
	{
		setOpeData( &data, 0, velocita/1000, 0, sceneDepth[indexScene], 0, 0, sceneNoiseFile[indexScene]);
		WriteOpeData(&data);
	}
	return HD_CALLBACK_CONTINUE;
}

/**
 * Calculates updated object transform for drag object based on changes to
 * proxy transform.
 */
void updateDragObjectTransform()
{
    assert(gCurrentDragObj >= 0 && 
           gCurrentDragObj < draggableObjects.size());

    // Calculated delta between current proxy pos and proxy pos at start 
    // of drag.
    hduVector3Dd proxyPos;
    hlGetDoublev(HL_PROXY_POSITION, proxyPos);
    hduVector3Dd dragDeltaTransl = proxyPos - gStartDragProxyPos;

    // Same for rotation.
    hduMatrix deltaRotMat;
    if (gRotate)
    {
        hduQuaternion proxyRotq;
        hlGetDoublev(HL_PROXY_ROTATION, proxyRotq);
        hduQuaternion dragDeltaRot = gStartDragProxyRot.inverse() * proxyRotq;
        dragDeltaRot.normalize();
        dragDeltaRot.toRotationMatrix(deltaRotMat);

        // Want to rotate about the proxy position, not the origin,
        // so need to translate to/from proxy pos.
        hduMatrix toProxy = hduMatrix::createTranslation(-gStartDragProxyPos);
        hduMatrix fromProxy = hduMatrix::createTranslation(gStartDragProxyPos);
        deltaRotMat = toProxy * deltaRotMat * fromProxy;
    }

    // Compose rotation and translation deltas.
    hduMatrix deltaMat = deltaRotMat * hduMatrix::createTranslation(dragDeltaTransl);

    // Apply these deltas to the drag object transform.
    draggableObjects[gCurrentDragObj].transform = gStartDragObjTransform * deltaMat;
}

/**
 *Init the memory used to transfer data to and form PD
 */
void InitMemory(void)
{
	int errorCreate = CreatePhantomMemory();
	OpeData Nuovo;
	setOpeData( &Nuovo, 0, 0, 0, 0, 0, 0, 0 );
	pdSurface = 0.0;
	int errorOpen= OpenPhantomMemory();
	WriteOpeData(&Nuovo);
	WriteSurface(&pdSurface);
}


/**
 * Init the text file holding experiment's results
 */
void InitOutputFile()
{
	time_t t;
	if (time(&t)==time_t(-1)){cerr<<"badtime"; exit(1);}
	tm* data= localtime(&t);
	ostringstream nomeFile;
	sec=data->tm_sec;
	nomeFile<<"Risultati-"<<(data->tm_year+1900)<<"-"<<(data->tm_mon+1)<<"-";
	nomeFile<<(data->tm_mday)<<"-"<<(data->tm_hour)<<"_"<<(data->tm_min)<<"_";
	nomeFile<<(data->tm_sec)<<"-"<<nome<<".txt";
	fileOut.open((nomeFile.str()).c_str());
}

/**
 * Method used to skip to next scenario
 */
void nextButton(int)
{
	#if (defined(_HAPTIC_))
	int button = ( expRad->get_int_val() );
	int button2 = ( expRad2->get_int_val() );
	
	if ( indexScene == 0 )
	{
		fileOut<<"NOME: "<<nome<<" ETA': "<<eta<<endl;
	}

	fileOut<<"Scena "<<indexScene<<":";
	fileOut<<" depth="<<sceneDepth[indexScene];
	fileOut<<" noiseFile="<<sceneNoiseFile[indexScene];
	fileOut<<" Ruvidita="<<button;
	fileOut<<" Confidenza="<<button2;
	fileOut<<endl;
	if ( indexScene == EXP_NUMBER-1 )
	{
		setOpeData( &data, 0, 0.01, 0, sceneDepth[indexScene], 0, 0, sceneNoiseFile[indexScene]);
		WriteOpeData(&data);
		cout<<"Grazie per la collaborazione!"<<endl;
		exit(0);
		int errorClose = ClosePhantomMemory();
	}

	indexScene++;	
	drawPrompts();
	
	// print on screen the current scene properties
	cout<<"Scena "<<indexScene<<":"<<endl;
	cout<<" depth="<<sceneDepth[indexScene]<<endl;
	cout<<" noiseFile="<<sceneNoiseFile[indexScene]<<endl;
	#endif
}

#if(defined(_TEXTURE))
void loadTexture(void)
{
	// Load texture
	GLbyte *pBytes;
    GLint iWidth, iHeight, iComponents;
    GLenum eFormat;
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	glEnable(TEXTURETYPE);
	glGenTextures(TEXTURE_COUNT,textures);
	glBindTexture(TEXTURETYPE, textures[0]);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    pBytes = gltLoadTGA(textureFiles[0], &iWidth, &iHeight, &iComponents, &eFormat);
	gluBuild2DMipmaps(TEXTURETYPE, iComponents, iWidth, iHeight, eFormat, GL_UNSIGNED_BYTE, pBytes);
    
	glTexParameteri(TEXTURETYPE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(TEXTURETYPE, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(TEXTURETYPE,GL_TEXTURE_WRAP_S,GL_REPEAT);
	glTexParameteri(TEXTURETYPE,GL_TEXTURE_WRAP_T,GL_REPEAT);
	free(pBytes);
}
#endif

#if(defined(_SHADOWMAPPING))
void ShadowMappingFirst(hduVector3Dd cam)
{
    // Track camera angle
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    //gluPerspective(45.0f, 1.0f, 1.0f, 1000.0f);
	gluPerspective(40.0f, 1.0f, 274.0f, 1899.0f);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cam[0], cam[1], cam[2], 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    glViewport(0, 0, windowWidth, windowHeight);
 
    // Track light position
    glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

    // Clear the window with current clearing color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLfloat sPlane[4] = {1.0f, 0.0f, 0.0f, 0.0f};
    GLfloat tPlane[4] = {0.0f, 1.0f, 0.0f, 0.0f};
    GLfloat rPlane[4] = {0.0f, 0.0f, 1.0f, 0.0f};
    GLfloat qPlane[4] = {0.0f, 0.0f, 0.0f, 1.0f};

	glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuseLight);

    // Set up shadow comparison
    glEnable(TEXTURETYPE);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glTexParameteri(TEXTURETYPE, GL_TEXTURE_COMPARE_MODE, 
                       GL_COMPARE_R_TO_TEXTURE);
    glTexParameteri(TEXTURETYPE, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(TEXTURETYPE, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // Set up the eye plane for projecting the shadow map on the scene
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
    glEnable(GL_TEXTURE_GEN_R);
    glEnable(GL_TEXTURE_GEN_Q);
    glTexGenfv(GL_S, GL_EYE_PLANE, sPlane);
    glTexGenfv(GL_T, GL_EYE_PLANE, tPlane);
    glTexGenfv(GL_R, GL_EYE_PLANE, rPlane);
    glTexGenfv(GL_Q, GL_EYE_PLANE, qPlane);
}

void ShadowMappingSecond()
{
	glDisable(GL_ALPHA_TEST);
    glDisable(TEXTURETYPE);
    glDisable(GL_TEXTURE_GEN_S);
    glDisable(GL_TEXTURE_GEN_T);
    glDisable(GL_TEXTURE_GEN_R);
    glDisable(GL_TEXTURE_GEN_Q);
    
    if (glGetError() != GL_NO_ERROR)
        fprintf(stderr, "GL Error!\n");

    // Flush drawing commands
    glutSwapBuffers();
}

void ShadowMappingInit()
{
	const GLubyte *version;

	// Make sure required functionality is available!
    version = glGetString(GL_VERSION);
    if (((version[0] != '1') || (version[1] != '.') || 
         (version[2] < '4') || (version[2] > '9')) &&   // 1.4+
        (!gltIsExtSupported("GL_ARB_shadow")))
    {
        fprintf(stderr, "Neither OpenGL 1.4 nor GL_ARB_shadow"
                        " extension is available!\n");
        Sleep(2000);
        exit(0);
    }

    // Check for optional extension
    if (gltIsExtSupported("GL_ARB_shadow_ambient"))
    {
        ambientShadowAvailable = GL_TRUE;
    }
    else
    {
        fprintf(stderr, "GL_ARB_shadow_ambient extension not available!\n");
        fprintf(stderr, "Extra ambient rendering pass will be required.\n\n");
        Sleep(2000);
    }

   // Black background
    //glClearColor(0.0f, 0.0f, 0.0f, 1.0f );

    // Hidden surface removal
    //glEnable(GL_DEPTH_TEST);
    //glDepthFunc(GL_LEQUAL);
    glPolygonOffset(factor, 0.0f);

    // Set up some lighting state that never changes
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHT0);

	glEnable(TEXTURETYPE);

    // Set up some texture state that never changes
    glGenTextures(1, &shadowTextureID);
    glBindTexture(TEXTURETYPE, shadowTextureID);
    glTexParameteri(TEXTURETYPE, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(TEXTURETYPE, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameteri(TEXTURETYPE, GL_DEPTH_TEXTURE_MODE, GL_INTENSITY);
    if (ambientShadowAvailable)
        glTexParameterf(TEXTURETYPE, GL_TEXTURE_COMPARE_FAIL_VALUE_ARB, 
                        0.5f);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);
    glTexGeni(GL_Q, GL_TEXTURE_GEN_MODE, GL_EYE_LINEAR);

    RegenerateShadowMap(light0_position, shadowSize);
}

// Called to regenerate the shadow map
void RegenerateShadowMap(GLfloat light[], GLint shadowsize)
{
    GLfloat lightToSceneDistance, nearPlane, fieldOfView;
    GLfloat lightModelview[16], lightProjection[16];

    // Save the depth precision for where it's useful
    lightToSceneDistance = sqrt(light[0] * light[0] + 
                                light[1] * light[1] + 
                                light[2] * light[2]);
    nearPlane = lightToSceneDistance - 150.0f;
    if (nearPlane < 50.0f)
        nearPlane = 50.0f;
    // Keep the scene filling the depth texture
    fieldOfView = 17000.0f / lightToSceneDistance;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fieldOfView, 1.0f, nearPlane/4, nearPlane + 300.0f);
	//gluPerspective(40.0f, 1.0f, 247.748f, 1898.99f);
    glGetFloatv(GL_PROJECTION_MATRIX, lightProjection);
    // Switch to light's point of view
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(light[0], light[1], light[2], 
              0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
    glGetFloatv(GL_MODELVIEW_MATRIX, lightModelview);
    glViewport(0, 0, shadowsize, shadowsize);

    // Clear the window with current clearing color
    glClear(GL_DEPTH_BUFFER_BIT);

    // All we care about here is resulting depth values
    glShadeModel(GL_FLAT);
	//glShadeModel(GL_SMOOTH);
    //glDisable(GL_LIGHTING);
    //glDisable(GL_COLOR_MATERIAL);
    //glDisable(GL_NORMALIZE);
    //glColorMask(0, 0, 0, 0);

    // Overcome imprecision
    glEnable(GL_POLYGON_OFFSET_FILL);

    // Draw objects in the scene
    drawScene();

    // Copy depth values into depth texture
    glCopyTexImage2D(TEXTURETYPE, 0, GL_DEPTH_COMPONENT, 0, 0, shadowsize, shadowsize, 0);
	//glCopyTexImage2D(TEXTURETYPE, 0, GL_DEPTH_COMPONENT16, traslx, trasly, 256, 256, 0);

    // Restore normal drawing state
    glShadeModel(GL_SMOOTH);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glEnable(GL_NORMALIZE);
    glColorMask(1, 1, 1, 1);
    glDisable(GL_POLYGON_OFFSET_FILL);

    // Set up texture matrix for shadow map projection
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glTranslatef(0.5f, 0.5f, 0.5f);
    glScalef(0.5f, 0.5f, 0.5f);
    glMultMatrixf(lightProjection);
    glMultMatrixf(lightModelview);
}

#endif


/******************************************************************************
 Draws string prompts at the bottom of the screen.
******************************************************************************/
void drawPrompts()
{
    glPushAttrib(GL_ENABLE_BIT);
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0, gViewportWidth, 0, gViewportHeight, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    glRasterPos2f(windowWidth-20, windowHeight-20);
	sprintf(sceneString, "%d", indexScene);
	glutBitmapCharacter (GLUT_BITMAP_TIMES_ROMAN_24, sceneString[0]);
	glutBitmapCharacter (GLUT_BITMAP_TIMES_ROMAN_24, sceneString[1]);
    //drawString(sceneString);
    //glRasterPos2f(4, 36);
    //drawString("Use mouse to rotate, pan, zoom.");
    //glRasterPos2f(4, 18);
    //drawString("(A) to toggle axis snap, (R) to toggle rotation.");
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glPopAttrib();
}

