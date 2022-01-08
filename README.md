# Azure Sphere RemoteX

Remote GPIO application.

## Clone

```
git clone --recurse-submodules https://github.com/Azure-Sphere-DevX/AzureSphereRemoteX.git
```



---

## Install the AzureSphereRemoteX Server

You can only deploy the AzureSphereRemote server from a Windows or Linux computer.

To install the AzureSphereRemote server, follow these steps:

1. Clone the AzureSphereRemoteX.Server repo to your desktop.

    ```
    git clone --recurse-submodules https://github.com/Azure-Sphere-DevX/AzureSphereRemoteX.Server.git
    ```

1. Start Visual Studio Code.
1. Open the AzureSphereRemoteX.Server folder with Visual Studio Code.
1. Open the **azsphere_board.txt** and select your developer board.
1. Press <kbd>F1</kbd>, and select **CMake: Select Variant.**, select **Release**
1. Press <kbd>F1</kbd>, and select **CMake: Delete Cache and Reconfigure**.
1. Set the app_manifest to match your Azure Sphere developer board. The RemoteX server project includes sample hardware definitions and matching app_manifests. Copy and paste the app_manifest from the **Manifests.defaults** folder that matches your board into the app_manifest.json file. 
1. Press <kbd>Ctrl+F5</kbd> to deploy the RemoteX server to your Azure Sphere.

### Expected device behavior

1. The application LED will turn on to indicate the RemoteX server is starting and connecting to your Wi-Fi network.
1. The application LED will turn off to indicate the RemoteX server is ready.
