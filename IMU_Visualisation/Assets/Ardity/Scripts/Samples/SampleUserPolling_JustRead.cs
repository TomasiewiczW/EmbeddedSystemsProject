/**
 * Ardity (Serial Communication for Arduino + Unity)
 * Author: Daniel Wilches <dwilches@gmail.com>
 *
 * This work is released under the Creative Commons Attributions license.
 * https://creativecommons.org/licenses/by/2.0/
 */

using System;
using UnityEngine;
using System.Collections;

/**
 * Sample for reading using polling by yourself. In case you are fond of that.
 */
public class SampleUserPolling_JustRead : MonoBehaviour
{
    public SerialController serialController;

    // Initialization
    void Start()
    {
        serialController = GameObject.Find("SerialController").GetComponent<SerialController>();
	}

    // Executed each frame
    void Update()
    {
        string message = serialController.ReadSerialMessage();

        if (message == null)
            return;
        // Check if the message is plain data or a connect/disconnect event.
        if (ReferenceEquals(message, SerialController.SERIAL_DEVICE_CONNECTED))
            Debug.Log("Connection established");
        else if (ReferenceEquals(message, SerialController.SERIAL_DEVICE_DISCONNECTED))
            Debug.Log("Connection attempt failed or disconnection detected");
        else
        {
            // Assuming that, message comes as (xRotation, yRotation, zRotation) - in euler angles
            string[] sArray = message.Split(' ');
            Vector3 designatedRotation = new Vector3(Int32.Parse(sArray[0]), Int32.Parse(sArray[1]), Int32.Parse(sArray[2]));
            Vector3 deltaRotation = new Vector3(designatedRotation.x - this.GetComponent<Transform>().rotation.eulerAngles.x, 0, 0);
            this.GetComponent<Transform>().Rotate(deltaRotation);
        }

    }
}
