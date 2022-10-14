# coding: utf-8

__all__ = ['GLSLangValidatorError']


class Error(Exception):
	"""Base class for exceptions in this module."""
	pass


class GLSLangValidatorError(Error):
	"""Exception raised for glslangValidator reported errors.

	Attributes:
		shader_name -- the atlas name that in processing
		msg  -- explanation of the error
	"""

	def __init__(self, shader_name, msg):
		self.shader_name=shader_name
		self.msg=msg
