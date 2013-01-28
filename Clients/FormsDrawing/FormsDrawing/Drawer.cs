using System;
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

namespace FormsDrawing
{
    public partial class Drawer : Form
    {
        Graphics g;
        Pen pen;

        bool painting = true;
        bool reading = false;
        bool newData = false;
        int numSwaps = 0;
        Thread readerThread;

        List<VisDataMessage> updateList = new List<VisDataMessage>();
        List<VisDataMessage> bufferList = new List<VisDataMessage>();
        List<VisDataMessage> drawList = new List<VisDataMessage>();

        double centreX = 0.0, focusX = 0.0;
        double centreY = 0.0, focusY = 0.0;
        double minExtent = 50.0;

        int mouseDownX = 0;
        int mouseDownY = 0;
        bool moving = false;

        Smarty smarty = new Smarty();
        Vector3D thrust = new Vector3D(0, 0, 0);

        public Drawer()
        {
            InitializeComponent();
            this.SetStyle(ControlStyles.OptimizedDoubleBuffer, true);
            this.SetStyle(ControlStyles.UserPaint, true);
            this.SetStyle(ControlStyles.AllPaintingInWmPaint, true);
            this.SetStyle(ControlStyles.ResizeRedraw, true);
            this.UpdateStyles();

            lblNumMessages.BackColor = Color.LightGray;
            lblViewCoords.BackColor = Color.LightGray;
            lblThrust.BackColor = Color.LightGray;

            g = this.CreateGraphics();
            pen = new Pen(Color.Black, 1);

            StartReaderThread(null);
        }

        void StartReaderThread(Smarty smarty)
        {
            readerThread = new Thread(new ParameterizedThreadStart(this.SocketReader));
            readerThread.Start(smarty);
            reading = true;
            stripReconnect.Enabled = false;
        }

        void SocketReader(object arg)
        {
            Smarty smarty = (Smarty)arg;
            int srv_id = -1;
            int client_id = -1;

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
                    srv = new IPEndPoint(IPAddress.Parse("127.0.0.1"), 5505);
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
                    if ((msg.id == -1) || ((updateList.Count > 0) && 
                        (msg.id < updateList[updateList.Count - 1].id)))
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

            s.Flush();
            s.Close();
            client.Close();
        }

        protected override void OnPaint(PaintEventArgs pe)
        {
        }

        private void Form1_Load(object sender, EventArgs e)
        {
        }

        private void timer1_Tick(object sender, EventArgs e)
        {
            if (!smarty.Connected())
            {
                stripSmarty.Text = "Connect Smarty";
            }

            if ((readerThread != null) && (!readerThread.IsAlive))
            {
                stripReconnect.Enabled = true;
            }

            if (painting)
            {
                List<VisDataMessage> tmp;

                g.Clear(Color.LightGray);

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

                lblViewCoords.Text = "(" + (int)(focusX - xExtent) + "," + (int)(focusX + xExtent) + ") , (" + (int)(focusY - yExtent) + "," + (int)(focusY + yExtent) + ")";

                foreach (VisDataMessage v in drawList)
                {
                    if (v.id == -1)
                    {
                        continue;
                    }

                    double rX = Math.Max(1.0, canvasWidth * v.radius / xExtent);
                    double rY = Math.Max(1.0, canvasHeight * v.radius / yExtent);
                    double x = (1 + (focusX - v.pX) / xExtent) * canvasWidth + border - rX;
                    double y = (1 + (focusY - v.pY) / yExtent) * canvasHeight + border - rY;

                    // Do some basic clipping testing.
                    if ((x + 2 * rX < 0) || (x - 2 * rX > this.ClientSize.Width))
                        continue;

                    if ((y + 2 * rY < 0) || (y - 2 * rY > this.ClientSize.Height))
                        continue;

                    g.DrawEllipse(pen, (float)x, (float)y, 2 * (float)rX, 2 * (float)rY);
                }

                this.lblNumMessages.Text = "" + numSwaps;
                //numSwaps = 0;

                if (newData)
                {
                    lock (bufferList)
                    {
                        tmp = bufferList;
                        bufferList = drawList;
                        drawList = tmp;
                        newData = false;
                    }
                }
            }
        }

        private void Form1_FormClosing(object sender, FormClosingEventArgs e)
        {
            reading = false;
            readerThread.Join();
        }

        private void Form1_ResizeEnd(object sender, EventArgs e)
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

        private void Form1_MouseDown(object sender, MouseEventArgs e)
        {
            mouseDownX = e.X;
            mouseDownY = e.Y;
            moving = true;
        }

        private void Form1_MouseUp(object sender, MouseEventArgs e)
        {
            moving = false;
            centreX = focusX;
            centreY = focusY;
        }

        private void Form1_MouseWheel(object sender, MouseEventArgs e)
        {
            if (e.Delta < 0)
            {
                minExtent *= 2;
            }
            else if (e.Delta > 0)
            {
                if (minExtent > 1/1000)
                {
                    minExtent /= 2;
                }
            }
        }

        private void stripReconnect_Click(object sender, EventArgs e)
        {
            numSwaps = 0;
            StartReaderThread((smarty.Connected() ? smarty : null));
        }

        private void stripeSmarty_Click(object sender, EventArgs e)
        {
            if (stripSmarty.Text == "Connect Smarty")
            {
                SmartyPicker smartyPicker = new SmartyPicker(smarty, this);
                smartyPicker.Show();
            }
            else
            {
                DialogResult res = MessageBox.Show("Do you want to remove the smarty from the universe as well as disconnect?", "Remove From universe", MessageBoxButtons.YesNoCancel);
                if (res == System.Windows.Forms.DialogResult.Yes)
                {
                    smarty.Disconnect(true);
                    stripSmarty.Text = "Connect Smarty";
                }
                else if (res == System.Windows.Forms.DialogResult.No)
                {
                    smarty.Disconnect(false);
                    stripSmarty.Text = "Connect Smarty";
                }
                else
                {
                    return;
                }

                // Now we stop the reader thread again, and start up a global one.
                reading = false;
                readerThread.Join();
                StartReaderThread(null);
            }
        }

        internal void PickerDone()
        {
            stripSmarty.Text = "Disconnect Smarty";
            lblThrust.Text = "( " + thrust.X + " , " + thrust.Y + " , " + thrust.Z + " )";

            // Now we stop the current visdata thread, and spin up a new one

            reading = false;
            readerThread.Join();
            numSwaps = 0;
            StartReaderThread(smarty);
        }

        private void Drawer_KeyDown(object sender, KeyEventArgs e)
        {
            if (!smarty.Connected())
            {
                return;
            }

            switch (e.KeyCode)
            {
                case Keys.W:
                    thrust.Y = 100000000;
                    break;

                case Keys.A:
                    thrust.X = 100000000;
                    break;

                case Keys.S:
                    thrust.Y = -100000000;
                    break;

                case Keys.D:
                    thrust.X = -100000000;
                    break;

                default:
                    break;
            }

            lblThrust.Text = "( " + thrust.X + " , " + thrust.Y + " , " + thrust.Z + " )";
            smarty.UpdateThrust(thrust);
            e.Handled = true;
        }

        private void Drawer_KeyUp(object sender, KeyEventArgs e)
        {
            if (!smarty.Connected())
            {
                return;
            }

            switch (e.KeyCode)
            {
                case Keys.W:
                    thrust.Y = 0;
                    break;

                case Keys.A:
                    thrust.X = 0;
                    break;

                case Keys.S:
                    thrust.Y = 0;
                    break;

                case Keys.D:
                    thrust.X = 0;
                    break;

                default:
                    break;
            }

            lblThrust.Text = "( " + thrust.X + " , " + thrust.Y + " , " + thrust.Z + " )";
            smarty.UpdateThrust(thrust);
            e.Handled = true;
        }
    }
}
