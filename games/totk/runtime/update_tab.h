/* The settings panel's 更新 page (Mac; included by settings_panel.h): what upstream did after this version and, with the AI
   summary on, whether that is worth updating. It reads update/summary.json in the game's data folder, the folder of
   TOTK_CONTROLS, which the launcher rewrites at each launch (app/update_summary.h), and shows its texts as plain text.
   Nothing is installed from here: a button copies the commands. A controller's L1 / R1 reach the page, left / right pick a
   button, A presses it and up / down scroll the text. */
#pragma once
#include <sys/stat.h>
#include <time.h>

#ifndef UPDATE_TAB_PASTEBOARD
#define UPDATE_TAB_PASTEBOARD NSPasteboard.generalPasteboard   /* a test passes its own, leaving the player's clipboard alone */
#endif
#define UPDATE_TAB_AI "update/ai"
enum { UPDATE_TAB_MAX_BYTES = 64 * 1024, UPDATE_TAB_STRIP_H = 46, UPDATE_TAB_SCROLL = 48 };

static NSString *update_tab_folder(void) {
 const char *controls=getenv("TOTK_CONTROLS");
 return controls&&*controls?[@(controls).stringByDeletingLastPathComponent stringByAppendingPathComponent:@"update"]:nil;
}
static NSData *update_tab_read(void) {
 NSString *path=[update_tab_folder() stringByAppendingPathComponent:@"summary.json"];if(!path)return nil;
 NSDictionary *about=[NSFileManager.defaultManager attributesOfItemAtPath:path error:nil];
 if(![about.fileType isEqualToString:NSFileTypeRegular]||about.fileSize>UPDATE_TAB_MAX_BYTES)return nil;
 NSData *data=[NSData dataWithContentsOfFile:path];return data.length<=UPDATE_TAB_MAX_BYTES?data:nil;
}
static BOOL update_tab_ai(void) { char word[16];return runtime_setting_word(UPDATE_TAB_AI,word,sizeof word)!=0; }
static BOOL update_tab_number(id v) { return [v isKindOfClass:NSNumber.class]&&CFGetTypeID((__bridge CFTypeRef)v)!=CFBooleanGetTypeID(); }
/* The document, or nil for anything that is not exactly what the launcher writes: the page then shows its empty state. */
static NSDictionary *update_tab_parse(NSData *data) {
 NSDictionary *d=data?[NSJSONSerialization JSONObjectWithData:data options:0 error:nil]:nil;
 if(![d isKindOfClass:NSDictionary.class]||!update_tab_number(d[@"version"])||![d[@"version"] isEqual:@1])return nil;
 for(NSString *k in @[@"key",@"installed",@"installed_date",@"status",@"verdict",@"reason",@"note",@"copy"])if(![d[k] isKindOfClass:NSString.class])return nil;
 for(NSString *k in @[@"total",@"generated_at"])if(!update_tab_number(d[k]))return nil;
 if(![d[@"points"] isKindOfClass:NSArray.class])return nil;
 for(id v in d[@"points"])if(![v isKindOfClass:NSString.class])return nil;
 if(![@[@"behind",@"latest",@"unknown"] containsObject:d[@"status"]]||![@[@"update",@"wait",@"skip",@""] containsObject:d[@"verdict"]])return nil;
 return d;
}
static NSString *update_tab_time(double at) {
 time_t t=(time_t)(at>0&&at<4e9?at:0);struct tm when;char text[16];
 if(!t||!localtime_r(&t,&when)||!strftime(text,sizeof text,"%m-%d %H:%M",&when))return @"";
 return [settings_text("内容更新于 ","Updated ") stringByAppendingString:@(text)];
}
static NSAttributedString *update_tab_text(NSDictionary *d, BOOL ai) {
 NSMutableAttributedString *all=[NSMutableAttributedString new];
 void (^add)(NSString *,NSFont *,NSColor *,CGFloat)=^(NSString *text,NSFont *font,NSColor *color,CGFloat indent){
  NSMutableParagraphStyle *style=[NSMutableParagraphStyle new];style.lineSpacing=2;style.paragraphSpacing=5;style.headIndent=indent;
  [all appendAttributedString:[[NSAttributedString alloc] initWithString:[(all.length?@"\n":@"") stringByAppendingString:text]
   attributes:@{NSFontAttributeName:font,NSForegroundColorAttributeName:color,NSParagraphStyleAttributeName:style}]];};
 NSFont *plain=[NSFont systemFontOfSize:12.5],*strong=[NSFont systemFontOfSize:13 weight:NSFontWeightSemibold],*small=[NSFont systemFontOfSize:11.5];
 NSColor *white=[NSColor colorWithCalibratedWhite:0.92 alpha:1],*dim=[NSColor colorWithCalibratedWhite:0.7 alpha:1],*accent=[NSColor colorWithCalibratedRed:0.7 green:0.95 blue:0.8 alpha:1];
 if(!d){add(settings_text("还没有更新信息，下次启动游戏时会查一次","No update information yet; it is checked the next time the game starts"),plain,white,0);return all;}
 NSString *verdict=d[@"verdict"],*reason=d[@"reason"],*installed=d[@"installed"],*date=d[@"installed_date"];BOOL behind=[d[@"status"] isEqualToString:@"behind"];
 if(installed.length){
  NSMutableString *line=[settings_text("当前版本 ","Installed ") mutableCopy];[line appendString:installed];
  if(date.length)[line appendFormat:@"%@%@%@",settings_text("（"," ("),date,settings_text("）",")")];
  long long total=[d[@"total"] longLongValue];
  if(behind)[line appendFormat:total==1?@(TOTK_UI(" · 上游之后 %lld 个提交"," · %lld commit upstream since")):@(TOTK_UI(" · 上游之后 %lld 个提交"," · %lld commits upstream since")),total];
  add(line,small,dim,0);
 }
 NSString *advice=[verdict isEqualToString:@"update"]?settings_text("建议：值得更新","Advice: worth updating"):[verdict isEqualToString:@"wait"]?settings_text("建议：可以等等","Advice: it can wait"):
  [verdict isEqualToString:@"skip"]?settings_text("建议：不用更新","Advice: no need to update"):nil;
 if(advice)add(reason.length?[NSString stringWithFormat:@"%@  %@",advice,reason]:advice,strong,accent,0);
 for(NSString *point in d[@"points"])add([@"•  " stringByAppendingString:point],plain,white,12);
 if([d[@"note"] length])add(d[@"note"],small,dim,0);
 if(behind&&[d[@"copy"] length])add(settings_text("复制的命令要在退出游戏后，到仓库文件夹里运行。","Quit the game, then run the copied commands in the repository folder."),small,dim,0);
 if(ai!=[d[@"key"] hasSuffix:@"|on"])add(ai?settings_text("AI 总结下次启动游戏时生效，它会用你自己的 AI 账号。","The AI summary starts the next time the game starts, and uses your own AI account."):
  settings_text("AI 总结下次启动游戏时关闭。","The AI summary stops the next time the game starts."),small,dim,0);
 return all;
}

@interface TOTKUpdatePage:NSView
@property(strong) NSScrollView *scroll;
@property(strong) NSTextField *text, *status, *time;
@property(strong) NSButton *commandButton, *aiSwitch;
@property(strong) NSDictionary *summary;   /* the document on screen; nil: the empty state */
@property(strong) NSData *shownData;
@property BOOL shown, shownAI;
-(void)refresh;
-(void)scrollBy:(CGFloat)distance;
@end
@implementation TOTKUpdatePage
-(NSButton *)buttonWithAction:(SEL)action{
 NSButton *button=[NSButton buttonWithTitle:@"" target:self action:action];button.refusesFirstResponder=YES;button.font=[NSFont systemFontOfSize:12];button.wantsLayer=YES;[self addSubview:button];return button;
}
-(instancetype)initWithFrame:(NSRect)frame{
 if(!(self=[super initWithFrame:frame]))return nil;
 CGFloat w=frame.size.width,h=frame.size.height;
 NSScrollView *scroll=[[NSScrollView alloc] initWithFrame:NSMakeRect(0,UPDATE_TAB_STRIP_H,w,fmax(0,h-UPDATE_TAB_STRIP_H))];
 scroll.hasVerticalScroller=YES;scroll.autohidesScrollers=YES;scroll.drawsBackground=NO;scroll.borderType=NSNoBorder;
 TOTKFlippedView *list=[[TOTKFlippedView alloc] initWithFrame:NSMakeRect(0,0,w,0)];
 NSTextField *text=[NSTextField wrappingLabelWithString:@""];text.selectable=NO;text.refusesFirstResponder=YES;[list addSubview:text];
 scroll.documentView=list;[self addSubview:scroll];self.scroll=scroll;self.text=text;
 NSTextField *status=[NSTextField labelWithString:@""];status.font=[NSFont systemFontOfSize:12 weight:NSFontWeightMedium];status.textColor=[NSColor colorWithCalibratedRed:0.7 green:0.95 blue:0.8 alpha:1];
 status.lineBreakMode=NSLineBreakByTruncatingTail;status.refusesFirstResponder=YES;status.frame=NSMakeRect(18,27,w-36,17);[self addSubview:status];self.status=status;
 NSTextField *time=[NSTextField labelWithString:@""];time.font=[NSFont systemFontOfSize:11];time.textColor=[NSColor colorWithCalibratedWhite:0.6 alpha:1];
 time.lineBreakMode=NSLineBreakByTruncatingTail;time.refusesFirstResponder=YES;[self addSubview:time];self.time=time;
 self.commandButton=[self buttonWithAction:@selector(copyClicked:)];self.aiSwitch=[self buttonWithAction:@selector(aiClicked:)];
 return self;
}
-(NSString *)statusLine{
 NSString *state=self.summary[@"status"];
 return [state isEqualToString:@"behind"]?settings_text("有新版本","Update available"):[state isEqualToString:@"latest"]?settings_text("已是最新","Up to date"):
  [state isEqualToString:@"unknown"]?settings_text("暂时查不到更新","Updates cannot be checked right now"):@"";
}
-(void)refresh{
 NSData *data=update_tab_read();BOOL ai=update_tab_ai();
 if(self.shown&&ai==self.shownAI&&(data==self.shownData||[data isEqual:self.shownData]))return;   /* the scroll position stays */
 self.shown=YES;self.shownAI=ai;self.shownData=data;self.summary=update_tab_parse(data);
 NSDictionary *d=self.summary;CGFloat w=self.frame.size.width;
 self.text.attributedStringValue=update_tab_text(d,ai);
 CGFloat width=fmax(0,w-36),height=ceil([self.text.cell cellSizeForBounds:NSMakeRect(0,0,width,CGFLOAT_MAX)].height),room=self.scroll.contentSize.height;
 self.text.frame=NSMakeRect(18,2,width,height);self.scroll.documentView.frame=NSMakeRect(0,0,w,fmax(height+8,room));
 [self.scroll.documentView scrollPoint:NSZeroPoint];
 self.status.stringValue=[self statusLine];
 self.time.stringValue=d?update_tab_time([d[@"generated_at"] doubleValue]):@"";
 self.commandButton.hidden=!([d[@"status"] isEqualToString:@"behind"]&&[d[@"copy"] length]);
 self.commandButton.title=settings_text("复制更新命令","Copy the update command");
 self.aiSwitch.title=ai?settings_text("关闭 AI 总结","Turn off the AI summary"):settings_text("开启 AI 总结","Turn on the AI summary");
 CGFloat right=w-14;
 for(NSButton *b in @[self.aiSwitch,self.commandButton]){if(b.hidden)continue;[b sizeToFit];CGFloat bw=b.frame.size.width;b.frame=NSMakeRect(right-bw,0,bw,26);right-=bw+6;}
 self.time.frame=NSMakeRect(18,5,fmax(0,right-18-6),15);
}
-(void)copyClicked:(id)sender{(void)sender;
 NSPasteboard *board=UPDATE_TAB_PASTEBOARD;[board clearContents];[board setString:self.summary[@"copy"] forType:NSPasteboardTypeString];
 self.status.stringValue=[NSString stringWithFormat:@"%@ · %@",[self statusLine],settings_text("命令已复制","command copied")];
 [self.window makeFirstResponder:self.window.contentView];   /* key 1 and Esc keep reaching the window */
}
-(void)aiClicked:(id)sender{(void)sender;
 if(update_tab_ai())runtime_setting_remove(UPDATE_TAB_AI);
 else{
  NSString *folder=update_tab_folder();if(folder)[NSFileManager.defaultManager createDirectoryAtPath:folder withIntermediateDirectories:YES attributes:nil error:nil];
  runtime_setting_save(UPDATE_TAB_AI,"on","AI summary");
 }
 [self refresh];
 [self.window makeFirstResponder:self.window.contentView];
}
-(void)scrollBy:(CGFloat)distance{
 NSClipView *clip=self.scroll.contentView;CGFloat most=fmax(0,self.scroll.documentView.frame.size.height-clip.bounds.size.height);
 [clip scrollToPoint:NSMakePoint(0,fmin(most,fmax(0,clip.bounds.origin.y+distance)))];[self.scroll reflectScrolledClipView:clip];
}
@end

@interface TOTKSettingsPanel (UpdateTab)
-(NSView *)updatePageWithFrame:(NSRect)frame;
-(void)refreshUpdatePage;
-(void)scrollUpdatePage:(int)step;
@end
@implementation TOTKSettingsPanel (UpdateTab)
-(NSView *)updatePageWithFrame:(NSRect)frame{self.updatePageTargets=[NSMutableArray array];return [[TOTKUpdatePage alloc] initWithFrame:frame];}
-(void)refreshUpdatePage{
 TOTKUpdatePage *page=(TOTKUpdatePage *)self.pages[SETTINGS_PAGE_UPDATE];[page refresh];
 [self.updatePageTargets removeAllObjects];
 for(NSButton *b in @[page.commandButton,page.aiSwitch])if(!b.hidden)[self.updatePageTargets addObject:b];
}
-(void)scrollUpdatePage:(int)step{[(TOTKUpdatePage *)self.pages[SETTINGS_PAGE_UPDATE] scrollBy:step*UPDATE_TAB_SCROLL];}
@end
