namespace FormsDrawing
{
    partial class SmartyPicker
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
            this.label1 = new System.Windows.Forms.Label();
            this.label2 = new System.Windows.Forms.Label();
            this.cmdClass = new System.Windows.Forms.Button();
            this.cmdShip = new System.Windows.Forms.Button();
            this.lstShips = new System.Windows.Forms.ListView();
            this.lstClasses = new System.Windows.Forms.ListView();
            this.cmdRename = new System.Windows.Forms.Button();
            this.txtName = new System.Windows.Forms.TextBox();
            this.cmdReady = new System.Windows.Forms.Button();
            this.SuspendLayout();
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(12, 9);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(33, 13);
            this.label1.TabIndex = 0;
            this.label1.Text = "Ships";
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(210, 9);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(67, 13);
            this.label2.TabIndex = 1;
            this.label2.Text = "Ship Classes";
            // 
            // cmdClass
            // 
            this.cmdClass.Location = new System.Drawing.Point(202, 240);
            this.cmdClass.Name = "cmdClass";
            this.cmdClass.Size = new System.Drawing.Size(75, 23);
            this.cmdClass.TabIndex = 4;
            this.cmdClass.Text = "Create Ship";
            this.cmdClass.UseVisualStyleBackColor = true;
            this.cmdClass.Click += new System.EventHandler(this.cmdClass_Click);
            // 
            // cmdShip
            // 
            this.cmdShip.Location = new System.Drawing.Point(12, 240);
            this.cmdShip.Name = "cmdShip";
            this.cmdShip.Size = new System.Drawing.Size(75, 23);
            this.cmdShip.TabIndex = 5;
            this.cmdShip.Text = "Join Ship";
            this.cmdShip.UseVisualStyleBackColor = true;
            this.cmdShip.Click += new System.EventHandler(this.cmdShip_Click);
            // 
            // lstShips
            // 
            this.lstShips.Location = new System.Drawing.Point(15, 25);
            this.lstShips.MultiSelect = false;
            this.lstShips.Name = "lstShips";
            this.lstShips.Size = new System.Drawing.Size(121, 209);
            this.lstShips.TabIndex = 6;
            this.lstShips.UseCompatibleStateImageBehavior = false;
            this.lstShips.View = System.Windows.Forms.View.List;
            // 
            // lstClasses
            // 
            this.lstClasses.Location = new System.Drawing.Point(156, 25);
            this.lstClasses.MultiSelect = false;
            this.lstClasses.Name = "lstClasses";
            this.lstClasses.Size = new System.Drawing.Size(121, 209);
            this.lstClasses.TabIndex = 7;
            this.lstClasses.UseCompatibleStateImageBehavior = false;
            this.lstClasses.View = System.Windows.Forms.View.List;
            // 
            // cmdRename
            // 
            this.cmdRename.Location = new System.Drawing.Point(12, 339);
            this.cmdRename.Name = "cmdRename";
            this.cmdRename.Size = new System.Drawing.Size(75, 23);
            this.cmdRename.TabIndex = 8;
            this.cmdRename.Text = "Rename Ship";
            this.cmdRename.UseVisualStyleBackColor = true;
            this.cmdRename.Click += new System.EventHandler(this.cmdRename_Click);
            // 
            // txtName
            // 
            this.txtName.Location = new System.Drawing.Point(15, 313);
            this.txtName.Name = "txtName";
            this.txtName.Size = new System.Drawing.Size(100, 20);
            this.txtName.TabIndex = 9;
            // 
            // cmdReady
            // 
            this.cmdReady.Location = new System.Drawing.Point(202, 313);
            this.cmdReady.Name = "cmdReady";
            this.cmdReady.Size = new System.Drawing.Size(75, 49);
            this.cmdReady.TabIndex = 10;
            this.cmdReady.Text = "Ready!";
            this.cmdReady.UseVisualStyleBackColor = true;
            this.cmdReady.Click += new System.EventHandler(this.cmdReady_Click);
            // 
            // SmartyPicker
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(289, 374);
            this.Controls.Add(this.cmdReady);
            this.Controls.Add(this.txtName);
            this.Controls.Add(this.cmdRename);
            this.Controls.Add(this.lstClasses);
            this.Controls.Add(this.lstShips);
            this.Controls.Add(this.cmdShip);
            this.Controls.Add(this.cmdClass);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.label1);
            this.Name = "SmartyPicker";
            this.Text = "SmartyPicker";
            this.Load += new System.EventHandler(this.SmartyPicker_Load);
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.Button cmdClass;
        private System.Windows.Forms.Button cmdShip;
        private System.Windows.Forms.ListView lstShips;
        private System.Windows.Forms.ListView lstClasses;
        private System.Windows.Forms.Button cmdRename;
        private System.Windows.Forms.TextBox txtName;
        private System.Windows.Forms.Button cmdReady;
    }
}