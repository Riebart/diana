namespace Diana2DClient
{
    partial class frmMain
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
            this.cmdSmarty = new System.Windows.Forms.Button();
            this.cmdGlobalVis = new System.Windows.Forms.Button();
            this.cmdVis = new System.Windows.Forms.Button();
            this.cmdHelm = new System.Windows.Forms.Button();
            this.cmdDisconnect = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // cmdSmarty
            // 
            this.cmdSmarty.Location = new System.Drawing.Point(12, 12);
            this.cmdSmarty.Name = "cmdSmarty";
            this.cmdSmarty.Size = new System.Drawing.Size(75, 37);
            this.cmdSmarty.TabIndex = 0;
            this.cmdSmarty.Text = "Pick a Ship";
            this.cmdSmarty.UseVisualStyleBackColor = true;
            this.cmdSmarty.Click += new System.EventHandler(this.cmdSmarty_Click);
            // 
            // cmdGlobalVis
            // 
            this.cmdGlobalVis.Location = new System.Drawing.Point(197, 12);
            this.cmdGlobalVis.Name = "cmdGlobalVis";
            this.cmdGlobalVis.Size = new System.Drawing.Size(75, 37);
            this.cmdGlobalVis.TabIndex = 1;
            this.cmdGlobalVis.Text = "Global Visualization";
            this.cmdGlobalVis.UseVisualStyleBackColor = true;
            this.cmdGlobalVis.Click += new System.EventHandler(this.cmdGlobalVis_Click);
            // 
            // cmdVis
            // 
            this.cmdVis.Enabled = false;
            this.cmdVis.Location = new System.Drawing.Point(12, 126);
            this.cmdVis.Name = "cmdVis";
            this.cmdVis.Size = new System.Drawing.Size(75, 23);
            this.cmdVis.TabIndex = 2;
            this.cmdVis.Text = "Visualization";
            this.cmdVis.UseVisualStyleBackColor = true;
            this.cmdVis.Click += new System.EventHandler(this.cmdVis_Click);
            // 
            // cmdHelm
            // 
            this.cmdHelm.Enabled = false;
            this.cmdHelm.Location = new System.Drawing.Point(12, 155);
            this.cmdHelm.Name = "cmdHelm";
            this.cmdHelm.Size = new System.Drawing.Size(75, 23);
            this.cmdHelm.TabIndex = 3;
            this.cmdHelm.Text = "Helm";
            this.cmdHelm.UseVisualStyleBackColor = true;
            this.cmdHelm.Click += new System.EventHandler(this.cmdHelm_Click);
            // 
            // cmdDisconnect
            // 
            this.cmdDisconnect.Enabled = false;
            this.cmdDisconnect.Location = new System.Drawing.Point(12, 55);
            this.cmdDisconnect.Name = "cmdDisconnect";
            this.cmdDisconnect.Size = new System.Drawing.Size(75, 37);
            this.cmdDisconnect.TabIndex = 5;
            this.cmdDisconnect.Text = "Disconnect from Ship";
            this.cmdDisconnect.UseVisualStyleBackColor = true;
            this.cmdDisconnect.Click += new System.EventHandler(this.cmdDisconnect_Click);
            // 
            // frmMain
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(284, 261);
            this.Controls.Add(this.cmdDisconnect);
            this.Controls.Add(this.cmdHelm);
            this.Controls.Add(this.cmdVis);
            this.Controls.Add(this.cmdGlobalVis);
            this.Controls.Add(this.cmdSmarty);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "frmMain";
            this.Text = "Diana - 2D Client";
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.Button cmdSmarty;
        private System.Windows.Forms.Button cmdGlobalVis;
        private System.Windows.Forms.Button cmdVis;
        private System.Windows.Forms.Button cmdHelm;
        private System.Windows.Forms.Button cmdDisconnect;
    }
}

