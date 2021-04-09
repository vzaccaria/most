
" ALE keybindings
nnoremap <Leader>d :ALEDetail<cr>
nmap <silent> [g <Plug>(ale_previous_wrap)
nmap <silent> ]g <Plug>(ale_next_wrap)
nmap <silent> <C-h> <Plug>(ale_detail)


" if exists('*ale#linter#Define')
" call ale#linter#Define('tex', {
" \   'name': 'vzredpen',
" \   'executable': 'vzredpen',
" \   'command': 'vzredpen -f latex -r json %t',
" \   'callback': 'ale#handlers#redpen#HandleRedpenOutput',
" \})
" endif

" if exists('*ale#linter#Define')
" call ale#linter#Define('lhaskell', {
" \   'name': 'lit-stack-ghc',
" \   'output_stream': 'stderr',
" \   'executable': function('ale#handlers#haskell#GetStackExecutable'),
" \   'command': function('ale_linters#haskell#stack_ghc#GetCommand'),
" \   'callback': 'ale#handlers#haskell#HandleGHCFormat',
" \})
" endif

"call ale#linter#Define('haskell', {
"\   'name': 'stack_ghc_local',
"\   'aliases': ['stack-ghc-local'],
"\   'output_stream': 'stderr',
"\   'executable': 'stack',
"\   'command': 'stack ghc -- -fno-code -v0 %t -hide-package cryptonite',
"\   'callback': 'ale#handlers#haskell#HandleGHCFormat',
"\})



if g:os == "Darwin" 

elseif g:os == "Linux" 
        let g:ale_fixers = {
        \   'c': ['clang-format'],
        \   'cpp': ['clang-format']
        \}

        let g:ale_fix_on_save = 1
        let g:ale_linters = {
                        \ 'cpp': []
                  \ }
endif



