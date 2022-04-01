
" ALE keybindings
nnoremap <Leader>d :ALEDetail<cr>
nmap <silent> [g <Plug>(ale_previous_wrap)
nmap <silent> ]g <Plug>(ale_next_wrap)
nmap <silent> <C-h> <Plug>(ale_detail)



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



