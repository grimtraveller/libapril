import os

import tkFileDialog
import tkMessageBox

from Tkinter import *

from jpt import Jpt

FILES_JPT = [("JPEG-PNG-Tarred files", ".jpt")]
FILES_JPEG = [("JPG files", ".jpg"), ("JPEG files", ".jpeg")]
FILES_PNG = [("PNG files", ".png")]
FILES_CUSTOM = [("All files", ".*")]

class FrameJpt(Frame):
	
	def __init__(self, master, row, column, title, confirm, files):
		Frame.__init__(self, master, bd = 3, relief = "raised")
		self.pack()
		self.grid(row = row, column = column)
		self.title = title
		self.confirm = confirm
		self.files = files
		self.labels = []
		self.entries = []
		self.buttons = []
		
	def setup(self):
		self._create_elements()
		self._setup_elements()
		
	def _create_elements(self):
		self.label_title = Label(self, text = self.title)
		functions = (self._button_browse_0, self._button_browse_1, self._button_browse_2)
		for i in xrange(0, len(self.files)):
			self.labels.append(Label(self, text = self.files[i]))
			self.entries.append(Entry(self, width = 50))
			self.buttons.append(Button(self, text = "Select", command = functions[i]))
		self.button_confirm = Button(self, text = self.confirm, command = self._button_confirm)
		
	def _setup_elements(self):
		self.label_title.grid(padx = 6, pady = 4, row = 0, sticky = W, columnspan = 3)
		for i in xrange(0, len(self.labels)):
			self.labels[i].grid(padx = 6, pady = 4, row = i + 1, sticky = W)
			self.entries[i].grid(padx = 6, pady = 4, row = i + 1, column = 1)
			self.buttons[i].grid(padx = 6, pady = 4, row = i + 1, column = 2)
		self.button_confirm.grid(padx = 6, pady = 4, row = len(self.labels) + 1, sticky = W, column = 1)

	def _button_browse_0(self):
		self._browse(0)
		
	def _button_browse_1(self):
		self._browse(1)
		
	def _button_browse_2(self):
		self._browse(2)
			
	def _button_confirm(self):
		self._confirm()
			
	def _browse(self, index):
		result = self._check_browse(index)
		if result != None:
			self.entries[index].delete(0, END)
			self.entries[index].insert(0, result)
	
	def _check_browse(self, index):
		return None
		
	def _confirm(self):
		pass
		
	def _check_file_save(self, index):
		filename = self.entries[index].get()
		if filename == "":
			tkMessageBox.showwarning(self.files[index], "The filename was not specified.")
			return False
		return True
		
	def _check_file_open(self, index):
		filename = self.entries[index].get()
		if filename == "":
			tkMessageBox.showwarning(self.files[index], "The filename was not specified.")
			return False
		if not os.path.exists(filename):
			tkMessageBox.showwarning(self.files[index], "The file '%s' does not exist." % filename)
			return False
		return True
		
		
class MergeJpt(FrameJpt):
	
	def __init__(self, master, row, column):
		FrameJpt.__init__(self, master, row, column, "Merge JPT from JPEG + PNG", "Merge JPT", ("JPT File", "JPEG File", "PNG File"))
	
	def _check_browse(self, index):
		if index == 0:
			return tkFileDialog.asksaveasfilename(defaultextension = FILES_JPT[0][1], filetypes = FILES_JPT)
		if index == 1:
			return tkFileDialog.askopenfilename(defaultextension = FILES_JPEG[0][1], filetypes = FILES_JPEG)
		if index == 2:
			return tkFileDialog.askopenfilename(defaultextension = FILES_PNG[0][1], filetypes = FILES_PNG)
		return FrameJpt._check_browse(index)
		
	def _confirm(self):
		if not self._check_file_save(0):
			return
		if not self._check_file_open(1):
			return
		if not self._check_file_open(2):
			return
		result = Jpt.merge(self.entries[0].get(), self.entries[1].get(), self.entries[2].get())
		tkMessageBox.showinfo(self.title, result)
		
		
class SplitJpt(FrameJpt):
	
	def __init__(self, master, row, column):
		FrameJpt.__init__(self, master, row, column, "Split JPT to JPEG + PNG", "Split JPT", ("JPT File", "JPEG File", "PNG File"))
	
	def _check_browse(self, index):
		if index == 0:
			return tkFileDialog.askopenfilename(defaultextension = FILES_JPT[0][1], filetypes = FILES_JPT)
		if index == 1:
			return tkFileDialog.asksaveasfilename(defaultextension = FILES_JPEG[0][1], filetypes = FILES_JPEG)
		if index == 2:
			return tkFileDialog.asksaveasfilename(defaultextension = FILES_PNG[0][1], filetypes = FILES_PNG)
		return FrameJpt._check_browse(index)
		
	def _confirm(self):
		if not self._check_file_open(0):
			return
		if not self._check_file_save(1):
			return
		if not self._check_file_save(2):
			return
		result = Jpt.split(self.entries[0].get(), self.entries[1].get(), self.entries[2].get())
		tkMessageBox.showinfo(self.title, result)
		
		
class JptGui:

	def __init__(self, master):
		self.frame = Frame(master, bd = 6)
		self.frame.pack()
		self.merge = MergeJpt(self.frame, 0, 0)
		self.merge.setup()
		self.split = SplitJpt(self.frame, 1, 0)
		self.split.setup()
		
root = Tk()
root.title("JPT GUI")
root.iconbitmap(default = 'jpt-gui.ico')
root.resizable(False, False)
gui = JptGui(root)
root.mainloop()
