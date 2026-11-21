using System;
using System.IO;
using System.Diagnostics;
using System.Windows.Forms;
using System.Text;
using System.Runtime.InteropServices;
using System.IO.Compression;
using System.Reflection;
class Program {
 [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern uint GetShortPathName(string p, StringBuilder b, uint n);
 static string Short(string p) { var b=new StringBuilder(32768); return GetShortPathName(p,b,(uint)b.Capacity)>0 ? b.ToString() : p; }
 static string Q(string s) { return "\""+s+"\""; }
 static int Run(string exe,string args) { var p=Process.Start(new ProcessStartInfo(exe,args){UseShellExecute=false,CreateNoWindow=true}); p.WaitForExit(); return p.ExitCode; }
 static void Extract(string name, string target) {
  using (var resource = Assembly.GetExecutingAssembly().GetManifestResourceStream(name)) {
   if (resource == null) throw new Exception("缺少程序资源：" + name);
   using (var gzip = new GZipStream(resource, CompressionMode.Decompress))
   using (var output = File.Create(target)) gzip.CopyTo(output);
  }
 }
 static string ConvertFile(string input,string format) {
  string dir=Path.GetDirectoryName(Path.GetFullPath(input)); string output=Path.Combine(dir,Path.GetFileNameWithoutExtension(input)+"."+format);
  if(File.Exists(output)) throw new Exception("输出文件已存在："+output);
  string tmp=Path.Combine(Path.GetTempPath(),"silk-"+Guid.NewGuid().ToString("N")); Directory.CreateDirectory(tmp);
  try { string i=Path.Combine(tmp,"input.silk"), pcm=Path.Combine(tmp,"audio.pcm"), result=Path.Combine(tmp,"result."+format); File.Copy(input,i);
   string t=Short(tmp); string exe=Path.Combine(tmp,"decoder.exe");
   Extract("decoder.gz",exe); Extract("ffmpeg.gz",Path.Combine(tmp,"ffmpeg.exe"));
   int code=Run(exe,Q(Path.Combine(t,"input.silk"))+" "+Q(Path.Combine(t,"audio.pcm")));
   if(code!=0 || !File.Exists(pcm) || new FileInfo(pcm).Length==0) throw new Exception("SILK 解码失败或文件无音频数据");
   code=Run(Path.Combine(tmp,"ffmpeg.exe"),"-nostdin -loglevel error -y -f s16le -ar 24000 -ac 1 -i "+Q(pcm)+" "+Q(result));
   if(code!=0 || !File.Exists(result) || new FileInfo(result).Length==0) throw new Exception("FFmpeg 转换失败");
   File.Copy(result,output,false); return output;
  } finally { try { Directory.Delete(tmp,true); } catch {} }
 }
 [STAThread] static int Main(string[] args) {
  if(args.Length>=2) { try { Console.WriteLine(ConvertFile(args[0],args[1])); return 0; } catch(Exception e) { Console.Error.WriteLine(e.Message); return 1; } }
  Application.EnableVisualStyles(); var f=new Form {Text="SILK 音频转换器",Width=620,Height=430,StartPosition=FormStartPosition.CenterScreen};
  var list=new ListBox {Left=15,Top=55,Width=570,Height=240,AllowDrop=true,HorizontalScrollbar=true};
  var add=new Button {Left=15,Top=15,Width=110,Text="添加文件"}; var folder=new Button {Left=135,Top=15,Width=110,Text="添加文件夹"};
  var format=new ComboBox {Left=260,Top=17,Width=100,DropDownStyle=ComboBoxStyle.DropDownList}; format.Items.AddRange(new object[]{"mp3","wav","ogg","flac"}); format.SelectedIndex=0;
  var start=new Button {Left=375,Top=15,Width=100,Text="开始转换"}; var clear=new Button {Left=485,Top=15,Width=100,Text="清空"};
  var status=new Label {Left=15,Top=310,Width=570,Height=65,Text="可拖入文件；转换结果保存在原文件旁。已存在的输出不会覆盖。"};
  Action<string> addPath = p => { if(File.Exists(p)) list.Items.Add(p); else if(Directory.Exists(p)) foreach(string x in Directory.GetFiles(p)) list.Items.Add(x); };
  add.Click+=(s,e)=>{var d=new OpenFileDialog {Multiselect=true,Filter="所有文件|*.*"}; if(d.ShowDialog()==DialogResult.OK) foreach(var x in d.FileNames) addPath(x);};
  folder.Click+=(s,e)=>{var d=new FolderBrowserDialog(); if(d.ShowDialog()==DialogResult.OK) addPath(d.SelectedPath);};
  list.DragEnter+=(s,e)=>{if(e.Data.GetDataPresent(DataFormats.FileDrop)) e.Effect=DragDropEffects.Copy;}; list.DragDrop+=(s,e)=>{foreach(string p in (string[])e.Data.GetData(DataFormats.FileDrop)) addPath(p);}; clear.Click+=(s,e)=>list.Items.Clear();
  start.Click+=(s,e)=>{start.Enabled=false; int ok=0,fail=0; var errors=new StringBuilder(); foreach(string p in list.Items) { status.Text="正在转换："+p; Application.DoEvents(); try {ConvertFile(p,format.Text); ok++;} catch(Exception ex) { fail++; errors.AppendLine(Path.GetFileName(p)+": "+ex.Message); } } status.Text="完成：成功 "+ok+"，失败 "+fail; if(fail>0) MessageBox.Show(errors.ToString(),"转换失败"); start.Enabled=true;};
  f.Controls.AddRange(new Control[]{list,add,folder,format,start,clear,status}); Application.Run(f); return 0;
 }
}
