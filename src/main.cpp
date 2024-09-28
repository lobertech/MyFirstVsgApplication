#include <vsg/all.h>

#ifdef vsgXchange_FOUND
#include <vsgXchange/all.h>
#endif

#include <iostream>

int main(int argc, char** argv)
{
    auto options = vsg::Options::create();
    options->paths = vsg::getEnvPaths("VSG_FILE_PATH");
    options->sharedObjects = vsg::SharedObjects::create();
    
    auto windowTraits = vsg::WindowTraits::create();
    windowTraits->windowTitle = "MyFirstVsgApplication";

    auto builder = vsg::Builder::create();
    builder->options = options;

    // set up defaults and read command line arguments to override them
    vsg::CommandLine arguments(&argc, argv);
    windowTraits->debugLayer = arguments.read({"--debug","-d"});
    windowTraits->apiDumpLayer = arguments.read({"--api","-a"});
    
    vsg::GeometryInfo geomInfo;
    geomInfo.dx.set(1.0f, 0.0f, 0.0f);
    geomInfo.dy.set(0.0f, 1.0f, 0.0f);
    geomInfo.dz.set(0.0f, 0.0f, 1.0f);
    geomInfo.cullNode = arguments.read("--cull");

    vsg::StateInfo stateInfo;
    
    if (arguments.read({"--fullscreen", "--fs"})) windowTraits->fullscreen = true;
    if (arguments.read({"--window", "-w"}, windowTraits->width, windowTraits->height)) { windowTraits->fullscreen = false; }
    arguments.read("--screen", windowTraits->screenNum);
    arguments.read("--display", windowTraits->display);

    if (arguments.read("--IMMEDIATE")) windowTraits->swapchainPreferences.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    if (arguments.read("--double-buffer")) windowTraits->swapchainPreferences.imageCount = 2;
    if (arguments.read("--triple-buffer")) windowTraits->swapchainPreferences.imageCount = 3; // default
    if (arguments.read("-t"))
    {
        windowTraits->swapchainPreferences.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
        windowTraits->width = 192, windowTraits->height = 108;
        windowTraits->decoration = false;
    }

    if (arguments.read("--shared")) options->sharedObjects = vsg::SharedObjects::create();

    auto outputFilename = arguments.value<vsg::Path>("", "-o");

    bool floatColors = !arguments.read("--ubvec4-colors");
    stateInfo.wireframe = arguments.read("--wireframe");
    stateInfo.lighting = !arguments.read("--flat");
    stateInfo.two_sided = arguments.read("--two-sided");

    vsg::vec4 specularColor;
    bool hasSpecularColor = arguments.read("--specular", specularColor);
    vsg::vec4 diffuseColor;
    bool hasDiffuseColor = arguments.read("--diffuse", diffuseColor);
    if (stateInfo.lighting && (hasDiffuseColor || hasSpecularColor))
    {
        builder->shaderSet = vsg::createPhongShaderSet(options);
        if (auto& materialBinding = builder->shaderSet->getDescriptorBinding("material"))
        {
            auto mat = vsg::PhongMaterialValue::create();
            if (hasSpecularColor)
            {
                vsg::info("specular = ", specularColor);
                mat->value().specular = specularColor;
            }
            if (hasDiffuseColor)
            {
                vsg::info("diffuse= ", diffuseColor);
                mat->value().diffuse = diffuseColor;
            }
            materialBinding.data = mat;
            vsg::info("using custom material ", mat);
        }
    }

    arguments.read("--dx", geomInfo.dx);
    arguments.read("--dy", geomInfo.dy);
    arguments.read("--dz", geomInfo.dz);
    
    bool billboard = arguments.read("--billboard");
    
    auto numVertices = arguments.value<uint32_t>(0, "-n");

    vsg::Path textureFile = arguments.value(vsg::Path{}, {"-i", "--image"});
    vsg::Path displacementFile = arguments.value(vsg::Path{}, "--dm");
    
    if (arguments.errors()) return arguments.writeErrorMessages(std::cerr);
    
#ifdef vsgXchange_all
    // add use of vsgXchange's support for reading and writing 3rd party file formats
    options->add(vsgXchange::all::create());
#endif

    auto scene = vsg::Group::create();
    
    vsg::dvec3 centre = {0.0, 0.0, 0.0};
    double radius = 1.0;

    radius = vsg::length(geomInfo.dx + geomInfo.dy + geomInfo.dz);

    //geomInfo.transform = vsg::perspective(vsg::radians(60.0f), 2.0f, 1.0f, 10.0f);
    //geomInfo.transform = vsg::inverse(vsg::perspective(vsg::radians(60.0f), 1920.0f/1080.0f, 1.0f, 100.0f)  * vsg::translate(0.0f, 0.0f, -1.0f) * vsg::scale(1.0f, 1.0f, 2.0f));
    //geomInfo.transform = vsg::rotate(vsg::radians(0.0), 0.0, 0.0, 1.0);

    if (textureFile) stateInfo.image = vsg::read_cast<vsg::Data>(textureFile, options);
    if (displacementFile) stateInfo.displacementMap = vsg::read_cast<vsg::Data>(displacementFile, options);

    vsg::dbox bound;

    if (numVertices > 0 || billboard)
    {
        if (numVertices == 0) numVertices = 1;

        if (billboard)
        {
            stateInfo.billboard = true;

            float w = std::pow(float(numVertices), 0.33f) * 2.0f * vsg::length(geomInfo.dx);
            float scaleDistance = w * 3.0f;
            auto positions = vsg::vec4Array::create(numVertices);
            geomInfo.positions = positions;
            for (auto& v : *(positions))
            {
                v.set(w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                      w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                      w * (float(std::rand()) / float(RAND_MAX) - 0.5f), scaleDistance);
            }

            radius += (0.5 * sqrt(3.0) * w);
        }
        else
        {
            stateInfo.instance_positions_vec3 = true;

            float w = std::pow(float(numVertices), 0.33f) * 2.0f * vsg::length(geomInfo.dx);
            auto positions = vsg::vec3Array::create(numVertices);
            geomInfo.positions = positions;
            for (auto& v : *(positions))
            {
                v.set(w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                      w * (float(std::rand()) / float(RAND_MAX) - 0.5f),
                      w * (float(std::rand()) / float(RAND_MAX) - 0.5f));
            }

            radius += (0.5 * sqrt(3.0) * w);
        }

        if (numVertices > 1)
        {
            if (floatColors)
            {
                stateInfo.instance_colors_vec4 = true;
                auto colors = vsg::vec4Array::create(numVertices);
                geomInfo.colors = colors;
                for (auto& c : *(colors))
                {
                    c.set(float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), float(std::rand()) / float(RAND_MAX), 1.0f);
                }
            }
            else
            {
                stateInfo.instance_colors_vec4 = false;
                auto colors = vsg::ubvec4Array::create(numVertices);
                geomInfo.colors = colors;
                for (auto& c : *(colors))
                {
                    c.set(uint8_t(255.0 * float(std::rand()) / float(RAND_MAX)), uint8_t(255.0 * float(std::rand()) / float(RAND_MAX)), uint8_t(255.0 * float(std::rand()) / float(RAND_MAX)), 255);
                }
            }
        }
    }
    
    // draw cylinder
    scene->addChild(builder->createCylinder(geomInfo, stateInfo));
    bound.add(geomInfo.position);
    geomInfo.position += geomInfo.dx * 1.5f;
    
    // update the centre and radius to account for all the shapes added so we can position the camera to see them all.
    centre = (bound.min + bound.max) * 0.5;
    radius += vsg::length(bound.max - bound.min) * 0.5;
    
    // read any vsg files from command line arguments
    /*for (int i=1; i<argc; ++i)
    {
        vsg::Path filename = arguments[i];
        auto loaded_scene = vsg::read_cast<vsg::Node>(filename, options);
        if (loaded_scene)
        {
            scene->addChild(loaded_scene);
            arguments.remove(i, 1);
            --i;
        }
    }

    if (scene->children.empty())
    {
        std::cout<<"No scene loaded, please specify valid 3d model(s) on command line."<<std::endl;
        return 1;
    }*/

    // write out scene if required
    if (outputFilename)
    {
        vsg::write(scene, outputFilename, options);
        return 0;
    }
    
    // create the viewer and assign window(s) to it
    auto viewer = vsg::Viewer::create();

    vsg::ref_ptr<vsg::Window> window(vsg::Window::create(windowTraits));
    if (!window)
    {
        std::cout<<"Could not create window."<<std::endl;
        return 1;
    }

    viewer->addWindow(window);

    // compute the bounds of the scene graph to help position the camera
    //vsg::ComputeBounds computeBounds;
    //scene->accept(computeBounds);
    //vsg::dvec3 centre = (computeBounds.bounds.min+computeBounds.bounds.max)*0.5;
    //double radius = vsg::length(computeBounds.bounds.max-computeBounds.bounds.min)*0.6;
    double nearFarRatio = 0.0001;

    // set up the camera
    auto lookAt = vsg::LookAt::create(centre+vsg::dvec3(0.0, -radius*3.5, 0.0), centre, vsg::dvec3(0.0, 0.0, 1.0));

    /*vsg::ref_ptr<vsg::ProjectionMatrix> perspective;
    if (vsg::ref_ptr<vsg::EllipsoidModel> ellipsoidModel(scene->getObject<vsg::EllipsoidModel>("EllipsoidModel")); ellipsoidModel)
    {
        // EllipsoidPerspective is useful for whole earth databases where per frame management of the camera's near & far values is optimized
        // to the current view relative to an ellipsoid model of the earth so that when near to the earth the near and far planes are pulled in close to the eye
        // and when far away from the earth's surface the far plane is pushed out to ensure that it encompasses the horizon line, accounting for mountains over the horizon.
        perspective = vsg::EllipsoidPerspective::create(lookAt, ellipsoidModel, 30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio, horizonMountainHeight);
    }
    else
    {
        perspective = vsg::Perspective::create(30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio*radius, radius * 4.5);
    }*/

    auto perspective = vsg::Perspective::create(30.0, static_cast<double>(window->extent2D().width) / static_cast<double>(window->extent2D().height), nearFarRatio * radius, radius * 10.0);
    
    auto camera = vsg::Camera::create(perspective, lookAt, vsg::ViewportState::create(window->extent2D()));

    // add close handler to respond to pressing the window's close window button and to pressing escape
    viewer->addEventHandler(vsg::CloseHandler::create(viewer));

    // add a trackball event handler to control the camera view using the mouse
    viewer->addEventHandler(vsg::Trackball::create(camera));

    // create a command graph to render the scene on the specified window
    auto commandGraph = vsg::createCommandGraphForView(window, camera, scene);
    viewer->assignRecordAndSubmitTaskAndPresentation({commandGraph});

    // compile all the Vulkan objects and transfer data required to render the scene
    viewer->compile();
    
    auto startTime = vsg::clock::now();
    double numFramesCompleted = 0.0;

    // rendering main loop
    while (viewer->advanceToNextFrame())
    {
        // pass any events into EventHandlers assigned to the Viewer
        viewer->handleEvents();

        viewer->update();

        viewer->recordAndSubmit();

        viewer->present();
        
        numFramesCompleted += 1.0;
    }

    auto duration = std::chrono::duration<double, std::chrono::seconds::period>(vsg::clock::now() - startTime).count();
    if (numFramesCompleted > 0.0)
    {
        std::cout << "Average frame rate = " << (numFramesCompleted / duration) << std::endl;
    }
    
    // clean up done automatically thanks to ref_ptr<>
    return 0;
}
