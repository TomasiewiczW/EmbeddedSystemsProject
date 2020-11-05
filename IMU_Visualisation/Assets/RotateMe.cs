using System.Collections;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using UnityEngine;


public class RotateMe: MonoBehaviour
{
    // Start is called before the first frame update
    void Start()
    {
        
    }

    // Update is called once per frame
    void Update()
    {
        if (Input.GetKeyDown(KeyCode.UpArrow))
        {
            transform.Rotate(Vector3.up);
        }
        if (Input.GetKeyDown(KeyCode.DownArrow))
        {
            transform.Rotate(Vector3.down);
        }
        if (Input.GetKeyDown(KeyCode.RightArrow))
        {
            transform.Rotate(Vector3.right);
        }
        if (Input.GetKeyDown(KeyCode.LeftArrow))
        {
            transform.Rotate(Vector3.left);
        }

    }
}
