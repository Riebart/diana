﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using System.IO;
using System.Net;
using System.Net.Sockets;
using System.Threading;


namespace Diana2DClient
{
    public partial class frmVisualization : Form
    {
        Graphics g;
        Pen pen;
        Pen reticlePen;

        bool painting = true;
        bool reading = false;
        bool newData = false;
        int numSwaps = 0;
        DateTime start_time;
        Thread readerThread;

        Color bgColor = Color.Black;
        Color fgColor = Color.White;
        Color rColor = Color.Red;

        List<VisDataMessage> updateList = new List<VisDataMessage>();
        List<VisDataMessage> bufferList = new List<VisDataMessage>();
        List<VisDataMessage> drawList = new List<VisDataMessage>();

        double centreX = 0.0, focusX = 0.0;
        double centreY = 0.0, focusY = 0.0;
        double minExtent = 50.0;

        int mouseDownX = 0;
        int mouseDownY = 0;
        bool moving = false;

        Smarty smarty;
        Vector3D thrust = new Vector3D(0, 0, 0);
        IPEndPoint srv = null;

        internal frmVisualization(IPEndPoint srv, Smarty smarty)
        {
            this.smarty = smarty;
            this.srv = srv;

            InitializeComponent();
            this.SetStyle(ControlStyles.OptimizedDoubleBuffer, true);
            this.SetStyle(ControlStyles.UserPaint, true);
            this.SetStyle(ControlStyles.AllPaintingInWmPaint, true);
            this.SetStyle(ControlStyles.ResizeRedraw, true);
            this.UpdateStyles();

            lblNumMessages.BackColor = bgColor;
            lblNumMessages.ForeColor = fgColor;
            lblViewCoords.BackColor = bgColor;
            lblViewCoords.ForeColor = fgColor;
            lblThrust.BackColor = bgColor;
            lblThrust.ForeColor = fgColor;

            g = this.CreateGraphics();
            pen = new Pen(fgColor, 1);
            reticlePen = new Pen(rColor, 1);

            start_time = DateTime.Now;

            StartReaderThread(smarty);
        }

        void StartReaderThread(Smarty smarty)
        {
            readerThread = new Thread(new ParameterizedThreadStart(this.SocketReader));
            reading = true;
            readerThread.Start(smarty);
        }

        void SocketReader(object arg)
        {
            Smarty smarty = (Smarty)arg;
            Int64 srv_id = -1;
            Int64 client_id = -1;

            if (smarty != null)
            {
                srv_id = smarty.osim_id;
                client_id = smarty.client_id;
            }

            TcpClient client;
            Stream s;
            client = new TcpClient();
            client.ReceiveTimeout = 1000;

            try
            {
                IPEndPoint srv;

                if (smarty != null)
                {
                    srv = smarty.ServerEndpoint();
                }
                else
                {
                    //srv = new IPEndPoint(IPAddress.Parse("127.0.0.1"), 5505);
                    srv = this.srv;
                }

                client.Connect(srv);
            }
            catch (System.Net.Sockets.SocketException)
            {
                MessageBox.Show("Unable to connect");
                return;
            }

            s = client.GetStream();

            if (smarty == null)
            {
                VisDataEnableMessage.Send(s, srv_id, client_id, true);
            }
            else
            {
                smarty.ConnectToVisData(s);
            }

            VisDataMessage msg;
            List<VisDataMessage> tmp;

            while (this.reading)
            {
                bool hungup = (client.Client.Poll(1, SelectMode.SelectRead) && (client.Available == 0));

                if (hungup)
                {
                    MessageBox.Show("The universe hung up on us.");
                    break;
                }

                Message rmsg = Message.GetMessage(s);

                if ((rmsg == null) || (rmsg.GetType() != typeof(VisDataMessage)))
                    continue;

                msg = (VisDataMessage)rmsg;

                if (msg != null)
                {
                    bool found = false;
                    foreach (VisDataMessage m in updateList)
                    {
                        if (m.id == msg.id)
                        {
                            found = true;
                            break;
                        }
                    }

                    if (found)
                    {
                        lock (bufferList)
                        {
                            tmp = bufferList;
                            bufferList = updateList;
                            updateList = tmp;
                            updateList.Clear();
                            numSwaps++;
                            newData = true;
                        }
                    }

                    updateList.Add(msg);
                }
            }

            if (smarty == null)
            {
                if (client.Connected)
                {
                    VisDataEnableMessage.Send(s, -1, -1, false);
                }
            }
            else
            {
                VisDataEnableMessage.Send(s, smarty.osim_id, smarty.client_id, false);
            }

            // There's a weird race condition where this force-closes the socket, and the data doesn't always make it to the
            // application on the remote end. 250ms should be enough time for the message to get processed. This is bad though.
            Thread.Sleep(250);
            s.Flush();
            s.Close();
            client.Close();
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (painting)
            {
                if (newData)
                {
                    lock (bufferList)
                    {
                        List<VisDataMessage> tmp;
                        tmp = bufferList;
                        bufferList = drawList;
                        drawList = tmp;
                        newData = false;
                    }

                    g.Clear(bgColor);

                    int border = 0;
                    int canvasWidth = this.ClientSize.Width / 2 - border;
                    int canvasHeight = this.ClientSize.Height / 2 - border;

                    double xExtent, yExtent;

                    if (this.ClientSize.Width < this.ClientSize.Height)
                    {
                        xExtent = minExtent;
                        yExtent = ((this.ClientSize.Height * 1.0 / this.ClientSize.Width) * minExtent);
                    }
                    else
                    {
                        xExtent = ((this.ClientSize.Width * 1.0 / this.ClientSize.Height) * minExtent);
                        yExtent = minExtent;
                    }

                    if (moving)
                    {
                        Point cursorPos = this.PointToClient(Cursor.Position);

                        focusX = centreX - 2 * (mouseDownX - cursorPos.X) * (xExtent / this.ClientSize.Width);
                        focusY = centreY - 2 * (mouseDownY - cursorPos.Y) * (yExtent / this.ClientSize.Height);
                    }
                    else
                    {
                        focusX = centreX;
                        focusY = centreY;
                    }

                    lblViewCoords.Text = "(" + (Int64)(focusX - xExtent) + "," + (Int64)(focusX + xExtent) + ") , (" + (Int64)(focusY - yExtent) + "," + (Int64)(focusY + yExtent) + ") @ " + (Int64)xExtent + "x" + (Int64)yExtent;
                    lblNumMessages.BackColor = bgColor;
                    lblNumMessages.ForeColor = fgColor;

                    foreach (VisDataMessage v in drawList)
                    {
                        if (v.id == -1)
                        {
                            continue;
                        }

                        double rX = Math.Max(1.0, canvasWidth * v.radius / xExtent);
                        double rY = Math.Max(1.0, canvasHeight * v.radius / yExtent);
                        double x = (1 + (focusX - v.position.X) / xExtent) * canvasWidth + border - rX;
                        double y = (1 + (focusY - v.position.Y) / yExtent) * canvasHeight + border - rY;

                        // Do some basic clipping testing.
                        if ((x + 2 * rX < 0) || (x - 2 * rX > this.ClientSize.Width))
                            continue;

                        if ((y + 2 * rY < 0) || (y - 2 * rY > this.ClientSize.Height))
                            continue;

                        try
                        {
                            g.DrawEllipse(pen, (float)x, (float)y, 2 * (float)rX, 2 * (float)rY);
                        }
                        catch(OverflowException)
                        {

                        }
                    }

                    g.DrawEllipse(reticlePen, this.ClientSize.Width / 2, this.ClientSize.Height / 2, 4, 4);

                    // Then draw the centre reticle

                    lblNumMessages.Text = "" + numSwaps + " (" + (int)(1000 * DateTime.Now.Subtract(start_time).TotalSeconds / numSwaps) + " ms)";
                    lblNumMessages.BackColor = bgColor;
                    lblNumMessages.ForeColor = fgColor;
                    //numSwaps = 0;
                }
            }
        }

        private void frmVisualization_FormClosing(object sender, FormClosingEventArgs e)
        {
            reading = false;
            readerThread.Join();
        }

        private void frmVisualization_ResizeEnd(object sender, EventArgs e)
        {
            g = this.CreateGraphics();
        }

        protected override void WndProc(ref System.Windows.Forms.Message m)
        {
            base.WndProc(ref m);

            if (m.Msg == 0x0112) // WM_SYSCOMMAND
            {
                // Check your window state here
                // SC_RESTORE is 0xF120, and SC_MINIMIZE is 0XF020
                if (((int.Parse(m.WParam.ToString()) & 0xFFF0) == 0xF120) ||
                    ((int.Parse(m.WParam.ToString()) & 0xFFF0) == 0xF010) || // This is what is thrown if you double-click the title bar
                    ((int.Parse(m.WParam.ToString()) & 0xFFF0) == 0xF030)) // Maximize event - SC_MAXIMIZE from Winuser.h
                {
                    g = this.CreateGraphics();
                }
            }
        }

        private void frmVisualization_MouseDown(object sender, MouseEventArgs e)
        {
            mouseDownX = e.X;
            mouseDownY = e.Y;
            moving = true;
        }

        private void frmVisualization_MouseUp(object sender, MouseEventArgs e)
        {
            moving = false;
            centreX = focusX;
            centreY = focusY;
        }

        private void frmVisualization_MouseWheel(object sender, MouseEventArgs e)
        {
            if (e.Delta < 0)
            {
                minExtent *= 2;
            }
            else if (e.Delta > 0)
            {
                if (minExtent > 1 / 1000)
                {
                    minExtent /= 2;
                }
            }
        }
    }
}
