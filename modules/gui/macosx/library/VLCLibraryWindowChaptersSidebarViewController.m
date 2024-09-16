/*****************************************************************************
 * VLCLibraryWindowChaptersSidebarViewController.m: MacOS X interface module
 *****************************************************************************
 * Copyright (C) 2024 VLC authors and VideoLAN
 *
 * Authors: Claudio Cambra <developer@claudiocambra.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#import "VLCLibraryWindowChaptersSidebarViewController.h"

#import "library/VLCLibraryDataTypes.h"
#import "main/VLCMain.h"
#import "playlist/VLCPlayerController.h"
#import "playlist/VLCPlaylistController.h"

@implementation VLCLibraryWindowChaptersSidebarViewController

- (instancetype)initWithLibraryWindow:(VLCLibraryWindow *)libraryWindow
{
    return [super initWithLibraryWindow:libraryWindow
                                nibName:@"VLCLibraryWindowChaptersView"];
}

- (void)viewDidLoad
{
    [super viewDidLoad];

    _chaptersArrayController = [[NSArrayController alloc] init];

    [self.tableView bind:NSContentBinding
                toObject:self.chaptersArrayController
             withKeyPath:@"arrangedObjects" 
                 options:nil];
    [self.tableView bind:NSSelectionIndexesBinding 
                toObject:self.chaptersArrayController
             withKeyPath:@"selectionIndexes"
                 options:nil];
    [self.tableView bind:NSSortDescriptorsBinding 
                toObject:self.chaptersArrayController
             withKeyPath:@"sortDescriptors"
                 options:nil];

    NSNotificationCenter * const notificationCenter = NSNotificationCenter.defaultCenter;
    [notificationCenter addObserver:self
                           selector:@selector(titleListChanged:)
                               name:VLCPlayerTitleListChanged
                             object:nil];
}

- (void)titleListChanged:(NSNotification *)notification
{
    VLCPlayerController * const playerController =
        VLCMain.sharedInstance.playlistController.playerController;

    const struct vlc_player_title * const title = playerController.selectedTitle;
    if (title == NULL) {
        self.chaptersArrayController.content = @[];
        return;
    }
    const struct vlc_player_chapter * const chapters = title->chapters;
    const size_t chapterCount = title->chapter_count;

    NSMutableArray * const chaptersArray = [NSMutableArray arrayWithCapacity:chapterCount];
    for (size_t i = 0; i < chapterCount; i++) {
        struct vlc_player_chapter chapter = chapters[i];
        [chaptersArray addObject:[NSString stringWithFormat:@"%zu. %@", i + 1, @(chapter.name)]];
    }
    self.chaptersArrayController.content = chaptersArray.copy;
}

@end
