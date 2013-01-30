namespace Diana2DClient
{
    partial class frmHelm
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
            this.trackRotation = new System.Windows.Forms.TrackBar();
            this.trackThrust = new System.Windows.Forms.TrackBar();
            this.lblRotation = new System.Windows.Forms.Label();
            this.lblThrust = new System.Windows.Forms.Label();
            this.picCompass = new System.Windows.Forms.PictureBox();
            this.timer1 = new System.Windows.Forms.Timer(this.components);
            ((System.ComponentModel.ISupportInitialize)(this.trackRotation)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackThrust)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.picCompass)).BeginInit();
            this.SuspendLayout();
            // 
            // trackRotation
            // 
            this.trackRotation.Location = new System.Drawing.Point(12, 519);
            this.trackRotation.Maximum = 360;
            this.trackRotation.Name = "trackRotation";
            this.trackRotation.Size = new System.Drawing.Size(499, 45);
            this.trackRotation.TabIndex = 0;
            this.trackRotation.TickFrequency = 10;
            this.trackRotation.TickStyle = System.Windows.Forms.TickStyle.Both;
            this.trackRotation.Scroll += new System.EventHandler(this.trackRotation_Scroll);
            // 
            // trackThrust
            // 
            this.trackThrust.Location = new System.Drawing.Point(520, 12);
            this.trackThrust.Maximum = 100;
            this.trackThrust.Name = "trackThrust";
            this.trackThrust.Orientation = System.Windows.Forms.Orientation.Vertical;
            this.trackThrust.Size = new System.Drawing.Size(45, 500);
            this.trackThrust.TabIndex = 1;
            this.trackThrust.TickFrequency = 5;
            this.trackThrust.TickStyle = System.Windows.Forms.TickStyle.Both;
            this.trackThrust.Scroll += new System.EventHandler(this.trackThrust_Scroll);
            // 
            // lblRotation
            // 
            this.lblRotation.AutoSize = true;
            this.lblRotation.Location = new System.Drawing.Point(517, 551);
            this.lblRotation.Name = "lblRotation";
            this.lblRotation.Size = new System.Drawing.Size(17, 13);
            this.lblRotation.TabIndex = 3;
            this.lblRotation.Text = "0°";
            this.lblRotation.TextAlign = System.Drawing.ContentAlignment.TopRight;
            // 
            // lblThrust
            // 
            this.lblThrust.AutoSize = true;
            this.lblThrust.Location = new System.Drawing.Point(517, 515);
            this.lblThrust.Name = "lblThrust";
            this.lblThrust.Size = new System.Drawing.Size(21, 13);
            this.lblThrust.TabIndex = 4;
            this.lblThrust.Text = "0%";
            // 
            // picCompass
            // 
            this.picCompass.BackgroundImage = global::Diana2DClient.Properties.Resources.Compass;
            this.picCompass.InitialImage = null;
            this.picCompass.Location = new System.Drawing.Point(12, 12);
            this.picCompass.Name = "picCompass";
            this.picCompass.Size = new System.Drawing.Size(500, 500);
            this.picCompass.TabIndex = 2;
            this.picCompass.TabStop = false;
            // 
            // timer1
            // 
            this.timer1.Enabled = true;
            this.timer1.Interval = 20;
            this.timer1.Tick += new System.EventHandler(this.timer1_Tick);
            // 
            // frmHelm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(567, 570);
            this.Controls.Add(this.lblThrust);
            this.Controls.Add(this.lblRotation);
            this.Controls.Add(this.picCompass);
            this.Controls.Add(this.trackThrust);
            this.Controls.Add(this.trackRotation);
            this.DoubleBuffered = true;
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "frmHelm";
            this.Text = "Helm";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.frmHelm_FormClosing);
            this.KeyDown += new System.Windows.Forms.KeyEventHandler(this.frmHelm_KeyDown);
            this.KeyUp += new System.Windows.Forms.KeyEventHandler(this.frmHelm_KeyUp);
            ((System.ComponentModel.ISupportInitialize)(this.trackRotation)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.trackThrust)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.picCompass)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.TrackBar trackRotation;
        private System.Windows.Forms.TrackBar trackThrust;
        private System.Windows.Forms.PictureBox picCompass;
        private System.Windows.Forms.Label lblRotation;
        private System.Windows.Forms.Label lblThrust;
        private System.Windows.Forms.Timer timer1;
    }
}