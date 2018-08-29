(
 (c-mode . ((c-file-style . "linux")
            (indent-tabs-mode . t)
            (tab-width . 4)
            (c-basic-offset . 4)))
 (c++-mode . (
              (c-file-style . "linux")
              (indent-tabs-mode . t)
              (c-basic-offset . 4)
              (c-basic-offset . 4)
              (tab-width . 4)
              (eval . (add-to-list 'c-offsets-alist '(innamespace . 0)))
              (eval . (add-to-list 'c-offsets-alist '(case-label . +)))
              ;; Indent classes with access labels - code from https://stackoverflow.com/questions/14715689/emacs-different-indentation-for-class-and-struct
              (eval . (c-set-offset 'access-label '-))
              (eval . (c-set-offset 'inclass (lambda (langelem)
                                               (save-excursion
                                                (c-beginning-of-defun)
                                                (if (or (looking-at "struct")
                                                        (looking-at "typedef struct"))
                                                    '+
                                                  '++)))))
              ))

 (python-mode . (
                 (indent-tabs-mode . t)))

 (nil . (
         (eval . (add-to-list 'auto-mode-alist '("\\.h\\'" . c++-mode)))
         ))
 )
