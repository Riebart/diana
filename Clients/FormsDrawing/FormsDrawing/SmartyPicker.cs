using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

using System.Net;

namespace FormsDrawing
{
    public partial class SmartyPicker : Form
    {
        Drawer parent;
        Smarty smarty;

        internal SmartyPicker(Smarty smarty, Drawer parent)
        {
            InitializeComponent();

            this.parent = parent;
            this.smarty = smarty;
        }

        private void SmartyPicker_Load(object sender, EventArgs e)
        {
            bool connected = smarty.Connect(new IPEndPoint(IPAddress.Loopback, 5506));

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

            int[] ids;
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

            int id = (int)lstClasses.SelectedItems[0].Tag;
            string name = lstClasses.SelectedItems[0].Name;

            int ship_id = smarty.CaptainNewShip(id, name);
            UpdateLists();
        }

        private void cmdShip_Click(object sender, EventArgs e)
        {
            if (lstShips.SelectedItems.Count == 0)
            {
                return;
            }

            int id = (int)lstShips.SelectedItems[0].Tag;
            string name = lstShips.SelectedItems[0].Name;

            int ship_id = smarty.JoinShip(id, name);
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
            parent.PickerDone();
            this.Close();
        }
    }
}
