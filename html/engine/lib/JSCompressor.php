<?php
	/**
	 * Merges every .js-file in directory/ies $dirs
	 * into one file and compresses them using
	 * Dead Edwards JavaScript-Packer
	 * 
	 * @class JSCompressor
	 */
	class JSCompressor {
		private $code;

		/**
		 * Gets all JS-code in directory/ies $dirs
		 *
		 * @method __construct
		 */
		public function __construct($dirs) {
			$this->getCodeFromDirs($dirs);
		}

		/**
		 * Packs the JS-code using Dean Edwards JS-packer
		 *
		 * @method pack
		 */
		public function pack() {
			$packer = new JavaScriptPacker($this->code);

			return $packer->pack();
		}

		/**
		 * Gets all JS-code in directory/ies $dirs
		 *
		 * @method getCodeFromDirs
		 */
		private function getCodeFromDirs($dirs) {
			$this->code = '';

			if(!is_array($dirs)) {
				$tmp = $dirs;
				$dirs = array();
				$dirs[] = $tmp;
			}

			foreach($dirs as $dir) {
				$dh = opendir($dir);
				if($dh) {
					while($f = readdir($dh)) {
						if('js' == end(explode('.', $f))) {
							$this->code .= file_get_contents("$dir/$f");
						}
					}
				}
			}
		}
	}
?>