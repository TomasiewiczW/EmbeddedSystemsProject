using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using UnityEngine;
using UnityEngine.UI;

public class RotateMe: MonoBehaviour
{
    public GameObject XAngleTextBox;
    public GameObject YAngleTextBox;
    public GameObject ZAngleTextBox;
    private Text Xtext;
    private Text Ytext;
    private Text Ztext;

    // Start is called before the first frame update
    void Start()
    {
        Xtext = XAngleTextBox.GetComponent<Text>();
        Ytext = YAngleTextBox.GetComponent<Text>();
        Ztext = ZAngleTextBox.GetComponent<Text>();
    }


    // Update is called once per frame
    void Update()
    {
        if (Input.GetKey(KeyCode.UpArrow))
        {
            transform.Rotate(Vector3.up * 30 * Time.deltaTime);
            updateTextDisplay();
        }
        if (Input.GetKey(KeyCode.DownArrow))
        {
            transform.Rotate(Vector3.down);
            updateTextDisplay();
        }
        if (Input.GetKey(KeyCode.RightArrow))
        {
            transform.Rotate(Vector3.right);
            updateTextDisplay();
        }
        if (Input.GetKey(KeyCode.LeftArrow))
        {
            transform.Rotate(Vector3.left);
            updateTextDisplay();
        }

    }
    
    void updateTextDisplay()
    {
        var rot = this.GetComponent<Transform>().rotation;
        Xtext.text = "X angle: " + rot.eulerAngles.x.ToString("0.00") + "deg";
        Ytext.text = "Y angle: " + rot.eulerAngles.y.ToString("0.00") + "deg";
        Ztext.text = "Z angle: " + rot.eulerAngles.z.ToString("0.00") + "deg";
    }
}
