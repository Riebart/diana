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
    public partial class frmSmartyPicker : Form
    {
        frmMain parent;
        Smarty smarty;
        IPEndPoint srv;

        internal frmSmartyPicker(IPEndPoint srv, frmMain parent)
        {
            InitializeComponent();

            this.parent = parent;
            this.srv = srv;
        }

        private void frmSmartyPicker_Load(object sender, EventArgs e)
        {
            smarty = new Smarty();
            bool connected = smarty.Connect(srv);

            if (!connected)
            {
                this.Close();
                return;
            }

            UpdateLists();
        }

        private void UpdateLists()
        {
            lstShips.Items.Clear();
            lstClasses.Items.Clear();

            Int64[] ids;
            string[] names;

            smarty.ListShips(out ids, out names);
            for (int i = 0; i < ids.Length; i++)
            {
                ListViewItem item = new ListViewItem(names[i]);
                item.Tag = ids[i];

                if (ids[i] == smarty.osim_id)
                {
                    txtName.Text = names[i];
                }

                lstShips.Items.Add(item);
            }

            smarty.ListClasses(out ids, out names);
            for (int i = 0; i < ids.Length; i++)
            {
                ListViewItem item = new ListViewItem(names[i]);
                item.Tag = ids[i];
                lstClasses.Items.Add(item);
            }
        }

        private void cmdClass_Click(object sender, EventArgs e)
        {
            if (lstClasses.SelectedItems.Count == 0)
            {
                return;
            }

            Int64 id = (Int64)lstClasses.SelectedItems[0].Tag;
            string name = lstClasses.SelectedItems[0].Name;

            Int64 ship_id = smarty.CaptainNewShip(id, name);
            UpdateLists();
        }

        private void cmdShip_Click(object sender, EventArgs e)
        {
            if (lstShips.SelectedItems.Count == 0)
            {
                return;
            }

            Int64 id = (Int64)lstShips.SelectedItems[0].Tag;
            string name = lstShips.SelectedItems[0].Name;

            Int64 ship_id = smarty.JoinShip(id, name);
            UpdateLists();
        }

        private void cmdRename_Click(object sender, EventArgs e)
        {
            smarty.RenameShip(txtName.Text);
            UpdateLists();
        }

        private void cmdReady_Click(object sender, EventArgs e)
        {
            smarty.Ready();
            parent.PickerDone(smarty);
            this.Close();
        }
    }
}
