using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

using System.IO;
using System.Net;
using System.Net.Sockets;

namespace Diana2DClient
{
    public partial class frmHelm : Form
    {
        Smarty smarty = null;
        TcpClient client = null;
        Stream s = null;
        Int64 clientID = -1;

        Bitmap needle;
        Graphics needleG;
        Pen p;

        PointF[] needlePts = new PointF[] { new PointF(0, 175), new PointF(119.287f, -128.046f), new PointF(0, -100), new PointF(-119.287f, -128.046f), new PointF(0, 175) };

        double angle = 0.0;
        double throttle = 0.0;

        double mouseAngle = 0.0;
        bool rotating = false;
        int rotatingBase;

        double mouseThrottle = 0.0;
        bool throttling = false;
        int throttleBase;

        internal frmHelm(Smarty smarty)
        {
            this.smarty = smarty;

            if (smarty != null)
            {
                clientID = smarty.GetConnection(out client, out s);
            }

            InitializeComponent();
            this.KeyPreview = true;

            needle = new Bitmap(picCompass.Width, picCompass.Height);
            needleG = Graphics.FromImage(needle);
            p = new Pen(Color.Red, 5);

            UpdateNeedle();
            UpdateNeedle();
        }

        // Assume we're rotating about the (250,250) centre
        private void RotatePoint(ref PointF p)
        {
            // {x Cos[t] + y Sin[t], y Cos[t] - x Sin[t]}
            float c = (float)Math.Cos(Math.PI * angle / 180);
            float s = (float)Math.Sin(Math.PI * angle / 180);
            float x = p.X * c + p.Y * s;
            float y = p.Y * c - p.X * s;
            p.X = x;
            p.Y = y;
        }

        private void UpdateNeedle()
        {
            double v = (((mouseAngle + angle) % 360) + 360) % 360;
            trackRotation.Value = (int)v;
            lblRotation.Text = (Math.Round(10 * v) / 10) + " °";

            needleG.Clear(Color.Transparent);
            needleG.DrawLines(p, needlePts);

            needleG.ResetTransform();
            needleG.TranslateTransform(250, 250);
            needleG.RotateTransform((float)(180 + mouseAngle + angle));

            picCompass.Image = needle;
        }

        private void UpdateThrottle()
        {
            double v = Math.Max(0, Math.Min(1, mouseThrottle + throttle));
            lblThrust.Text = (Math.Round(v * 1000) / 10) + " %";
            trackThrust.Value = (int)(100 * v);
        }

        private void frmHelm_KeyDown(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.A)
            {
                if (!rotating)
                {
                    rotatingBase = Cursor.Position.X;
                }

                rotating = true;
            }

            if (e.KeyCode == Keys.W)
            {
                if (!throttling)
                {
                    throttleBase = Cursor.Position.Y;
                }

                throttling = true;
            }
        }

        private void frmHelm_KeyUp(object sender, KeyEventArgs e)
        {
            if (e.KeyCode == Keys.A)
            {
                rotating = false;
                angle += mouseAngle;
                mouseAngle = 0;
                UpdateNeedle();
            }

            if (e.KeyCode == Keys.W)
            {
                throttling = false;
                throttle += mouseThrottle;
                mouseThrottle = 0;
                UpdateThrottle();
            }
        }

        private void trackRotation_Scroll(object sender, EventArgs e)
        {
            angle = trackRotation.Value;
            UpdateNeedle();
        }

        private void trackThrust_Scroll(object sender, EventArgs e)
        {
            throttle = trackThrust.Value / 100.0;
            UpdateThrottle();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (rotating)
            {
                mouseAngle = Cursor.Position.X - rotatingBase;
                UpdateNeedle();
            }

            if (throttling)
            {
                mouseThrottle = (throttleBase - Cursor.Position.Y) / 250.0;
                UpdateThrottle();
            }
        }

        private void frmHelm_FormClosing(object sender, FormClosingEventArgs e)
        {
            if (smarty != null)
            {
                smarty.Disconnect(client, s, clientID);
            }
        }
    }
}
