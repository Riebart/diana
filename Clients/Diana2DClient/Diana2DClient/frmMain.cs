using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using System.Net;

namespace Diana2DClient
{
    public partial class frmMain : Form
    {
        Smarty smarty = null;

        public frmMain()
        {
            InitializeComponent();
            frmHelm helm = new frmHelm(null);
            helm.Show();
        }

        internal void PickerDone(Smarty smarty)
        {
            this.smarty = smarty;

            SmartyButtonsToggle(smarty == null ? false : true);
        }

        private void SmartyButtonsToggle(bool enable)
        {
            cmdSmarty.Enabled = !enable;
            cmdDisconnect.Enabled = enable;

            cmdVis.Enabled = enable;
            cmdHelm.Enabled = enable;
        }

        private void cmdGlobalVis_Click(object sender, EventArgs e)
        {
            string result = Microsoft.VisualBasic.Interaction.InputBox("Enter the address and port in standard form of the universe to connect to.\n", "Universe Address", "127.0.0.1:5505");

            IPEndPoint srv = null;

            try
            {
                srv = StringToIPEndPoint(result);
            }
            catch (System.FormatException)
            {
                srv = null;
            }

            if (srv == null)
            {
                MessageBox.Show("Couldn't parse the entered address.");
            }
            else
            {
                frmVisualization globalVisForm = new frmVisualization(StringToIPEndPoint(result), null);
                globalVisForm.Show();
            }
        }

        private IPEndPoint StringToIPEndPoint(string result)
        {
            IPAddress addr = IPAddress.Parse(result.Substring(0, result.LastIndexOf(':')));
            int port = int.Parse(result.Substring(result.LastIndexOf(':') + 1));

            return new IPEndPoint(addr, port);
        }

        private void cmdSmarty_Click(object sender, EventArgs e)
        {
            string result = Microsoft.VisualBasic.Interaction.InputBox("Enter the address and port in standard form of the universe to connect to.\n", "Universe Address", "127.0.0.1:5506");

            IPEndPoint srv = null;

            try
            {
                srv = StringToIPEndPoint(result);
            }
            catch (System.FormatException)
            {
                srv = null;
            }

            if (srv == null)
            {
                MessageBox.Show("Couldn't parse the entered address.");
            }
            else
            {
                frmSmartyPicker picker = new frmSmartyPicker(srv, this);
                picker.Show();
            }
        }

        private void cmdDisconnect_Click(object sender, EventArgs e)
        {
            smarty.Disconnect(false);
            SmartyButtonsToggle(false);
        }

        private void cmdHelm_Click(object sender, EventArgs e)
        {
            frmHelm helm = new frmHelm(smarty);
            helm.Show();
        }

        private void cmdVis_Click(object sender, EventArgs e)
        {
            frmVisualization vis = new frmVisualization(null, smarty);
            vis.Show();
        }
    }
}
