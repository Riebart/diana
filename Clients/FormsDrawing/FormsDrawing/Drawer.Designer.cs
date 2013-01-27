namespace FormsDrawing
{
    partial class Drawer
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.lblNumMessages = new System.Windows.Forms.Label();
            this.lblViewCoords = new System.Windows.Forms.Label();
            this.toolStrip1 = new System.Windows.Forms.ToolStrip();
            this.stripReconnect = new System.Windows.Forms.ToolStripLabel();
            this.stripSmarty = new System.Windows.Forms.ToolStripLabel();
            this.lblThrust = new System.Windows.Forms.Label();
            this.toolStrip1.SuspendLayout();
            this.SuspendLayout();
            // 
            // timer1
            // 
            this.timer1.Enabled = true;
            this.timer1.Interval = 1;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // lblNumMessages
            // 
            this.lblNumMessages.AutoSize = true;
            this.lblNumMessages.Location = new System.Drawing.Point(12, 25);
            this.lblNumMessages.Name = "lblNumMessages";
            this.lblNumMessages.Size = new System.Drawing.Size(13, 13);
            this.lblNumMessages.TabIndex = 0;
            this.lblNumMessages.Text = "0";
            // 
            // lblViewCoords
            // 
            this.lblViewCoords.AutoSize = true;
            this.lblViewCoords.Location = new System.Drawing.Point(12, 38);
            this.lblViewCoords.Name = "lblViewCoords";
            this.lblViewCoords.Size = new System.Drawing.Size(38, 13);
            this.lblViewCoords.TabIndex = 1;
            this.lblViewCoords.Text = "Centre";
            // 
            // toolStrip1
            // 
            this.toolStrip1.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.stripReconnect,
            this.stripSmarty});
            this.toolStrip1.Location = new System.Drawing.Point(0, 0);
            this.toolStrip1.Name = "toolStrip1";
            this.toolStrip1.Size = new System.Drawing.Size(425, 25);
            this.toolStrip1.TabIndex = 2;
            this.toolStrip1.Text = "toolStrip1";
            // 
            // stripReconnect
            // 
            this.stripReconnect.Name = "stripReconnect";
            this.stripReconnect.Size = new System.Drawing.Size(63, 22);
            this.stripReconnect.Text = "Reconnect";
            this.stripReconnect.Click += new System.EventHandler(this.stripReconnect_Click);
            // 
            // stripSmarty
            // 
            this.stripSmarty.Name = "stripSmarty";
            this.stripSmarty.Size = new System.Drawing.Size(92, 22);
            this.stripSmarty.Text = "Connect Smarty";
            this.stripSmarty.Click += new System.EventHandler(this.stripeSmarty_Click);
            // 
            // lblThrust
            // 
            this.lblThrust.AutoSize = true;
            this.lblThrust.Location = new System.Drawing.Point(12, 51);
            this.lblThrust.Name = "lblThrust";
            this.lblThrust.Size = new System.Drawing.Size(37, 13);
            this.lblThrust.TabIndex = 3;
            this.lblThrust.Text = "Thrust";
            // 
            // Drawer
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(425, 409);
            this.Controls.Add(this.lblThrust);
            this.Controls.Add(this.toolStrip1);
            this.Controls.Add(this.lblViewCoords);
            this.Controls.Add(this.lblNumMessages);
            this.DoubleBuffered = true;
            this.Name = "Drawer";
            this.Text = "Form1";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.Form1_FormClosing);
            this.ResizeEnd += new System.EventHandler(this.Form1_ResizeEnd);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.Drawer_KeyDown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.Drawer_KeyUp);
            this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.Form1_MouseDown);
            this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.Form1_MouseUp);
            this.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.Form1_MouseWheel);
            this.toolStrip1.ResumeLayout(false);
            this.toolStrip1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Timer timer1;
        private System.Windows.Forms.Label lblNumMessages;
        private System.Windows.Forms.Label lblViewCoords;
        private System.Windows.Forms.ToolStrip toolStrip1;
        private System.Windows.Forms.ToolStripLabel stripReconnect;
        private System.Windows.Forms.ToolStripLabel stripSmarty;
        private System.Windows.Forms.Label lblThrust;
    }
}

