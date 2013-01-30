namespace Diana2DClient
{
    partial class frmVisualization
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
            this.lblThrust = new System.Windows.Forms.Label();
            this.lblViewCoords = new System.Windows.Forms.Label();
            this.lblNumMessages = new System.Windows.Forms.Label();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            this.SuspendLayout();
            // 
            // lblThrust
            // 
            this.lblThrust.AutoSize = true;
            this.lblThrust.Location = new System.Drawing.Point(12, 35);
            this.lblThrust.Name = "lblThrust";
            this.lblThrust.Size = new System.Drawing.Size(37, 13);
            this.lblThrust.TabIndex = 6;
            this.lblThrust.Text = "Thrust";
            // 
            // lblViewCoords
            // 
            this.lblViewCoords.AutoSize = true;
            this.lblViewCoords.Location = new System.Drawing.Point(12, 22);
            this.lblViewCoords.Name = "lblViewCoords";
            this.lblViewCoords.Size = new System.Drawing.Size(38, 13);
            this.lblViewCoords.TabIndex = 5;
            this.lblViewCoords.Text = "Centre";
            // 
            // lblNumMessages
            // 
            this.lblNumMessages.AutoSize = true;
            this.lblNumMessages.Location = new System.Drawing.Point(12, 9);
            this.lblNumMessages.Name = "lblNumMessages";
            this.lblNumMessages.Size = new System.Drawing.Size(13, 13);
            this.lblNumMessages.TabIndex = 4;
            this.lblNumMessages.Text = "0";
            // 
            // timer1
            // 
            this.timer1.Enabled = true;
            this.timer1.Interval = 1;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // frmVisualization
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(284, 261);
            this.Controls.Add(this.lblThrust);
            this.Controls.Add(this.lblViewCoords);
            this.Controls.Add(this.lblNumMessages);
            this.Name = "frmVisualization";
            this.Text = "Visualization";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmVisualization_FormClosing);
            this.ResizeEnd += new System.EventHandler(this.frmVisualization_ResizeEnd);
            this.MouseDown += new System.Windows.Forms.MouseEventHandler(this.frmVisualization_MouseDown);
            this.MouseUp += new System.Windows.Forms.MouseEventHandler(this.frmVisualization_MouseUp);
            this.MouseWheel += new System.Windows.Forms.MouseEventHandler(this.frmVisualization_MouseWheel);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblThrust;
        private System.Windows.Forms.Label lblViewCoords;
        private System.Windows.Forms.Label lblNumMessages;
        private System.Windows.Forms.Timer timer1;
    }
}