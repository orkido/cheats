import QtQuick
import QtQuick3D

Window {
    id: window
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowTransparentForInput | Qt.WindowStaysOnTopHint
    width: 1280
    height: 720
    visible: true
    color: "transparent"

    Component {
        id: basicRadar

        Node {
            Model {
                source: "#Sphere"
                position: Qt.vector3d(0, 0, 0)
                scale: Qt.vector3d(1, 1, 1)
                materials: DefaultMaterial {
                    diffuseColor: "yellow"
                }
            }
        }
    }

    Node {
        id: rootScene
        property var instances: []

        //Model {
        //    source: "#Sphere"
        //    position: Qt.vector3d(0, 0, 0)
        //    scale: Qt.vector3d(1, 1, 1)
        //    materials: DefaultMaterial {
        //        diffuseColor: "blue"
        //    }
        //}

        OrthographicCamera {
            id: orthographicCamera
            position: Qt.vector3d(0, 0, 4000)
            clipNear: 0.1
            clipFar: 10000
        }

        PerspectiveCamera {
            id: perspectiveCamera
            position: Qt.vector3d(0, 0, 4000)
            clipNear: 0.1
        }

        DirectionalLight {
            //ambientColor: Qt.rgba(0.5, 0.5, 0.5, 1.0)
            //brightness: 1.0
            rotation: orthographicCamera.rotation
        }
    }

    View3D {
        id: rootView
        anchors.fill: parent
        importScene: rootScene
        camera: orthographicCamera
    }

    function update_camera(fov: real, position: vector3d, rotation: vector3d, scale: vector3d) : bool {
        if (fov !== 0.0) {
            rootView.camera.fieldOfView = fov;
        }

        rootView.camera.position = position;
        rootView.camera.eulerRotation = rotation;
        rootView.camera.scale = scale;
        return true;
    }

    function update_camera_type(camera_type_perspective: bool) : bool {
        if (camera_type_perspective) {
            rootView.camera = perspectiveCamera;
        } else {
            rootView.camera = orthographicCamera;
        }
        return true;
    }

    function create_node(scale: real) : int {
        var node = basicRadar.createObject(rootScene);
        node.scale = Qt.vector3d(scale, scale, scale);
        rootScene.instances.push(node);
        return rootScene.instances.length - 1;
    }

    function delete_node(index: int) : bool {
        console.assert(index < rootScene.instances.length, "Delete: Invalid index");
        var node = rootScene.instances[index];
        rootScene.instances.splice(index, 1);
        node.destroy();
        return true;
    }

    function update_node(index: int, position: vector3d, rotation: vector3d) : bool {
        console.assert(index < rootScene.instances.length, "Update: Invalid index");
        var node = rootScene.instances[index];
        node.position = position;
        node.eulerRotation = rotation;
        return true;
    }
}
